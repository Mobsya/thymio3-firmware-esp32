import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest


GIT_HELPER = Path(__file__).resolve().parents[1] / "firmware_git.sh"
IDF_SOURCE = GIT_HELPER.parents[1] / ".deps/esp-adf_release2.4/esp-idf"


class FirmwareGitTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="firmware-git-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.env = {
            **os.environ,
            "GIT_CONFIG_GLOBAL": os.devnull,
            "GIT_CONFIG_NOSYSTEM": "1",
            "GIT_ALLOW_PROTOCOL": "file",
            "GIT_AUTHOR_NAME": "Firmware Test",
            "GIT_AUTHOR_EMAIL": "test@example.invalid",
            "GIT_COMMITTER_NAME": "Firmware Test",
            "GIT_COMMITTER_EMAIL": "test@example.invalid",
            "ADF_SKIP_UPDATE": "0",
            "DEPS_CLONE_DEPTH": "1",
        }
        self.leaf = self.new_repo("leaf")
        self.leaf_pin = self.commit(self.leaf, "pinned leaf")
        self.commit(self.leaf, "newer leaf")

        self.idf = self.new_repo("idf")
        self.git(self.idf, "submodule", "add", self.leaf.as_uri(), "nested")
        self.git(self.idf / "nested", "checkout", "--detach", self.leaf_pin)
        self.idf_pin = self.commit(self.idf, "pinned idf")
        self.commit(self.idf, "newer idf")

        self.adf = self.new_repo("adf")
        self.git(self.adf, "submodule", "add", self.idf.as_uri(), "esp-idf")
        self.git(self.adf / "esp-idf", "checkout", "--detach", self.idf_pin)
        self.adf_pin = self.commit(self.adf, "pinned adf")
        self.git(self.adf, "branch", "release/v2.4")
        self.git(self.adf, "tag", "-a", "v-fixture", "-m", "fixture", self.adf_pin)
        self.checkout = self.root / "checkout with spaces"
        self.env.update(
            ADF_REPO=self.adf.as_uri(),
            ADF_REF="release/v2.4",
            ADF_PATH=str(self.checkout),
        )

    def git(self, repo, *args):
        return subprocess.run(
            ["git", "-C", str(repo), *args],
            env=self.env,
            check=True,
            capture_output=True,
            text=True,
        ).stdout.strip()

    def new_repo(self, name):
        repo = self.root / name
        repo.mkdir()
        self.git(repo, "init", "--initial-branch=main")
        self.commit(repo, "initial")
        return repo

    def commit(self, repo, message):
        (repo / "content.txt").write_text(message + "\n", encoding="ascii")
        self.git(repo, "add", ".")
        self.git(repo, "commit", "-m", message)
        return self.git(repo, "rev-parse", "HEAD")

    def run_helper(self, command="checkout_adf_dependency", **overrides):
        return subprocess.run(
            ["bash", "-euc", 'source "$1"\n' + command, "bash", str(GIT_HELPER)],
            env={**self.env, **overrides},
            capture_output=True,
            text=True,
        )

    def checkout_ok(self, **overrides):
        result = self.run_helper(**overrides)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def assert_pins(self):
        for path, expected in (
            (self.checkout, self.adf_pin),
            (self.checkout / "esp-idf", self.idf_pin),
            (self.checkout / "esp-idf/nested", self.leaf_pin),
        ):
            self.assertEqual(self.git(path, "rev-parse", "HEAD"), expected)

    def test_shallow_clone_and_repeat_keep_pinned_nested_commits(self):
        self.checkout_ok()
        self.assert_pins()
        for path in (self.checkout, self.checkout / "esp-idf", self.checkout / "esp-idf/nested"):
            self.assertEqual(self.git(path, "rev-parse", "--is-shallow-repository"), "true")
        self.assertEqual(self.git(self.checkout, "tag", "--list"), "")
        self.checkout_ok()
        self.assert_pins()
        self.assertEqual(self.git(self.checkout, "rev-list", "--count", "HEAD"), "1")

    def test_branch_update_and_tag_switch(self):
        self.checkout_ok()
        self.git(self.adf, "checkout", "release/v2.4")
        self.git(self.adf / "esp-idf", "checkout", "--detach", "origin/main")
        updated_idf = self.git(self.adf / "esp-idf", "rev-parse", "HEAD")
        updated_adf = self.commit(self.adf, "updated adf")
        self.checkout_ok()
        self.assertEqual(self.git(self.checkout, "rev-parse", "HEAD"), updated_adf)
        self.assertEqual(self.git(self.checkout / "esp-idf", "rev-parse", "HEAD"), updated_idf)
        self.assertEqual(self.git(self.checkout, "rev-list", "--count", "HEAD"), "1")
        self.assertEqual(self.git(self.checkout, "tag", "--list"), "")
        self.checkout_ok(ADF_REF="v-fixture")
        self.assert_pins()

    def test_full_history_opt_out_for_fresh_clones(self):
        self.checkout_ok(DEPS_CLONE_DEPTH="0")
        self.assert_pins()
        for path in (self.checkout, self.checkout / "esp-idf", self.checkout / "esp-idf/nested"):
            self.assertEqual(self.git(path, "rev-parse", "--is-shallow-repository"), "false")
            self.assertGreater(int(self.git(path, "rev-list", "--count", "HEAD")), 1)

    def test_custom_depth(self):
        self.checkout_ok(DEPS_CLONE_DEPTH="2")
        self.assert_pins()
        self.assertEqual(self.git(self.checkout, "rev-list", "--count", "HEAD"), "2")

    def test_skip_update_preserves_existing_checkout(self):
        self.checkout_ok()
        self.git(self.adf, "checkout", "release/v2.4")
        self.commit(self.adf, "remote change")
        self.checkout_ok(ADF_SKIP_UPDATE="1")
        self.assert_pins()

    def test_invalid_depth_fails_before_cloning(self):
        for value in ("-1", "abc", "1 --tags", "01"):
            with self.subTest(depth=value):
                result = self.run_helper(DEPS_CLONE_DEPTH=value)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("DEPS_CLONE_DEPTH must be", result.stderr)
                self.assertFalse(self.checkout.exists())

    def test_local_edits_are_not_discarded(self):
        self.checkout_ok()
        edited = self.checkout / "content.txt"
        edited.write_text("local edits\n", encoding="ascii")
        self.git(self.adf, "checkout", "release/v2.4")
        self.commit(self.adf, "conflicting remote change")
        result = self.run_helper()
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(edited.read_text(encoding="ascii"), "local edits\n")

    def test_nonempty_directory_is_preserved(self):
        self.checkout.mkdir()
        marker = self.checkout / "local-file"
        marker.write_text("keep", encoding="ascii")
        result = self.run_helper()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("not an ESP-ADF git checkout", result.stderr)
        self.assertEqual(marker.read_text(encoding="ascii"), "keep")

    def make_firmware_source(self):
        micropython = self.new_repo("micropython")
        for path in ("lib/berkeley-db-1.xx", "lib/micropython-lib"):
            self.git(micropython, "submodule", "add", self.leaf.as_uri(), path)
            self.git(micropython / path, "checkout", "--detach", self.leaf_pin)
        self.git(micropython, "submodule", "add", self.idf.as_uri(), "lib/pico-sdk")
        micropython_pin = self.commit(micropython, "pinned micropython")
        self.commit(micropython, "newer micropython")

        firmware = self.new_repo("firmware")
        for path in ("components/esp32-wifi-manager", "components/thymio-nn"):
            self.git(firmware, "submodule", "add", self.leaf.as_uri(), path)
            self.git(firmware / path, "checkout", "--detach", self.leaf_pin)
        mp_path = "components/mp_component/micropython"
        self.git(firmware, "submodule", "add", micropython.as_uri(), mp_path)
        self.git(firmware / mp_path, "checkout", "--detach", micropython_pin)
        self.commit(firmware, "firmware dependencies")
        return firmware, micropython_pin

    def test_firmware_initializes_only_required_submodules_at_requested_depth(self):
        firmware, micropython_pin = self.make_firmware_source()
        for depth in ("1", "0"):
            with self.subTest(depth=depth):
                checkout = self.root / ("firmware checkout depth " + depth)
                self.git(self.root, "clone", "--no-recurse-submodules", firmware.as_uri(), str(checkout))
                mp = checkout / "components/mp_component/micropython"
                for _ in range(2):
                    result = self.run_helper(
                        'update_firmware_submodules "$FIRMWARE_PATH"',
                        FIRMWARE_PATH=str(checkout),
                        DEPS_CLONE_DEPTH=depth,
                    )
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                    for path, expected in (
                        (checkout / "components/esp32-wifi-manager", self.leaf_pin),
                        (checkout / "components/thymio-nn", self.leaf_pin),
                        (mp, micropython_pin),
                        (mp / "lib/berkeley-db-1.xx", self.leaf_pin),
                        (mp / "lib/micropython-lib", self.leaf_pin),
                    ):
                        self.assertEqual(self.git(path, "rev-parse", "HEAD"), expected)
                        self.assertEqual(
                            self.git(path, "rev-parse", "--is-shallow-repository"),
                            "true" if depth == "1" else "false",
                        )
                    self.assertFalse((mp / "lib/pico-sdk/.git").exists())
                    self.assertFalse((mp / "lib/pico-sdk/content.txt").exists())
                    mp_git_dir = Path(self.git(mp, "rev-parse", "--absolute-git-dir"))
                    self.assertFalse((mp_git_dir / "modules/lib/pico-sdk").exists())

    def test_firmware_preserves_existing_unused_submodule_edits(self):
        firmware, _ = self.make_firmware_source()
        checkout = self.root / "existing firmware checkout"
        self.git(self.root, "clone", "--recurse-submodules", firmware.as_uri(), str(checkout))
        unused = checkout / "components/mp_component/micropython/lib/pico-sdk"
        edited = unused / "content.txt"
        edited.write_text("local SDK edits\n", encoding="ascii")
        result = self.run_helper(
            'update_firmware_submodules "$FIRMWARE_PATH"',
            FIRMWARE_PATH=str(checkout),
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(edited.read_text(encoding="ascii"), "local SDK edits\n")
        self.assertTrue((unused / "nested/.git").exists())


@unittest.skipUnless((IDF_SOURCE / "tools/idf_tools.py").exists(), "requires local ESP-IDF checkout")
class ShallowIdfVersionTests(unittest.TestCase):
    def test_python_environment_version_without_git_tags(self):
        with tempfile.TemporaryDirectory(prefix="shallow-idf-") as temp:
            checkout = Path(temp) / "esp-idf"
            subprocess.run(
                ["git", "clone", "--depth", "1", "--no-tags", "--no-recurse-submodules",
                 IDF_SOURCE.as_uri(), str(checkout)],
                check=True,
                capture_output=True,
            )
            header = (checkout / "components/esp_common/include/esp_idf_version.h").read_text()
            major = re.search(r"^#define ESP_IDF_VERSION_MAJOR\s+(\d+)", header, re.M).group(1)
            minor = re.search(r"^#define ESP_IDF_VERSION_MINOR\s+(\d+)", header, re.M).group(1)
            result = subprocess.run(
                [sys.executable, "-c", """
import sys
sys.path.insert(0, sys.argv[1] + '/tools')
import idf_tools
idf_tools.global_idf_path = sys.argv[1]
idf_tools.global_idf_tools_path = sys.argv[2]
print(idf_tools.get_python_env_path()[0])
""", str(checkout), temp],
                check=True,
                capture_output=True,
                text=True,
                env={**os.environ, "PYTHONDONTWRITEBYTECODE": "1"},
            )
            self.assertIn("idf" + major + "." + minor + "_py", result.stdout)


if __name__ == "__main__":
    unittest.main()
