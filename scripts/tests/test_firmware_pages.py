import hashlib
from html.parser import HTMLParser
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
from urllib.error import HTTPError
from urllib.parse import quote, unquote, urlsplit
from urllib.request import Request


sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from build_firmware_pages import CatalogError, GitHubClient, SafeRedirectHandler, generate_site


BASE_URL = "https://mobsya.github.io/thymio3-firmware-esp32"


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def json_bytes(value):
    return json.dumps(value).encode()


class FixtureHTTP:
    """Exercise the real API pagination and download code without network access."""

    def __init__(self, client):
        self.client = client
        self.responses = {}
        self.requests = []

    def open(self, request, timeout):
        self.requests.append(request)
        response = self.responses.get(request.full_url)
        if response is None:
            raise HTTPError(request.full_url, 404, "Not Found", {}, None)
        if isinstance(response, Exception):
            raise response
        return io.BytesIO(response)

    def set_json(self, path, value):
        self.responses[self.client.api_root + path] = json_bytes(value)

    def set_pages(self, path, entries):
        for start in range(0, len(entries) + 1, 100):
            self.set_json(f"{path}?per_page=100&page={start // 100 + 1}", entries[start:start + 100])


class LinkParser(HTMLParser):
    def __init__(self):
        super().__init__()
        self.links = []

    def handle_starttag(self, tag, attrs):
        if tag == "a":
            self.links.append(dict(attrs)["href"])


class FirmwarePagesTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="test-firmware-pages-")
        self.addCleanup(self.temporary.cleanup)
        self.output = Path(self.temporary.name) / "site"
        self.client = GitHubClient("Mobsya/thymio3-firmware-esp32", token="fixture-token")
        self.http = FixtureHTTP(self.client)
        self.client.opener = self.http
        self.releases = []

    def add_release(self, release_id=1, files=None, **overrides):
        release = {
            "id": release_id,
            "tag_name": f"v{release_id}.0.0",
            "name": f"Release {release_id}",
            "draft": False,
            "prerelease": False,
            "published_at": f"2026-10-{min(release_id, 28):02d}T12:00:00Z",
            "html_url": f"https://github.com/Mobsya/thymio3-firmware-esp32/releases/{release_id}",
        }
        release.update(overrides)
        self.releases.append(release)
        assets = []
        for name, content in (files or {}).items():
            url = f"https://downloads.example/{release_id}/{quote(name, safe='')}"
            self.http.responses[url] = content
            assets.append({
                "name": name,
                "state": "uploaded",
                "size": len(content),
                "digest": f"sha256:{sha256(content)}",
                "browser_download_url": url,
            })
        self.http.set_pages(f"/releases/{release_id}/assets", assets)
        return release, assets

    def build(self, latest_id=None, **kwargs):
        self.http.set_pages("/releases", self.releases)
        if latest_id is not None:
            self.http.set_json("/releases/latest", next(r for r in self.releases if r["id"] == latest_id))
        return generate_site(self.client, self.output, BASE_URL, **kwargs)

    def firmware_files(self):
        # Include the historical date format: roles must come from the manifest.
        app_name = "ESP32-2026-10-05-872e8c2-1.10.2.bin"
        files = {app_name: b"\xe9\x00application\xff", "full.bin": b"\x00full\xff", "notes.txt": b"notes"}
        manifest = {
            "artifacts": [{"file": app_name, "role": "app", "size": len(files[app_name]), "sha256": sha256(files[app_name])}],
            "merged": {"file": "full.bin", "size": len(files["full.bin"]), "sha256": sha256(files["full.bin"])},
        }
        files["manifest.json"] = json_bytes(manifest)
        files["SHA256SUMS"] = "".join(f"{sha256(data)}  {name}\n" for name, data in files.items()).encode()
        return files

    def test_complete_mirror_and_public_contract(self):
        files = self.firmware_files()
        stable, _ = self.add_release(files=files)
        self.add_release(2, {"preview.bin": b"preview"}, prerelease=True)
        self.add_release(3, {"private.bin": b"private"}, draft=True, published_at=None)
        catalog = self.build(latest_id=stable["id"])
        self.assertEqual(catalog["schema_version"], 1)
        self.assertEqual(catalog["latest_stable"], "v1.0.0")
        self.assertEqual([r["id"] for r in catalog["releases"]], [2, 1])
        self.assertEqual(json.loads((self.output / "releases.json").read_text()), catalog)
        assets = catalog["releases"][1]["assets"]
        self.assertEqual(assets[0]["role"], "app")
        self.assertEqual(next(a for a in assets if a["name"] == "full.bin")["role"], "full")
        self.assertIsNone(next(a for a in assets if a["name"] == "notes.txt")["role"])
        for asset in assets:
            self.assertEqual(set(asset), {"name", "url", "size", "sha256", "role"})
            self.assertEqual((self.output / "releases/1" / asset["name"]).read_bytes(), files[asset["name"]])
            self.assertEqual(asset["size"], len(files[asset["name"]]))
            self.assertEqual(asset["sha256"], sha256(files[asset["name"]]))
        page = (self.output / "index.html").read_text()
        self.assertIn("Latest stable", page)
        self.assertIn("Prerelease", page)
        self.assertIn("Application firmware", page)
        self.assertIn("Full flash image", page)
        self.assertNotIn("private.bin", page)
        self.assertFalse((self.output / "releases/3").exists())
        parser = LinkParser()
        parser.feed(page)
        for link in parser.links:
            if link.startswith(BASE_URL + "/"):
                self.assertTrue((self.output / unquote(link[len(BASE_URL) + 1:])).is_file())
        self.assertFalse(any("/releases/3/assets" in r.full_url for r in self.http.requests))

    def test_latest_uses_github_selection_instead_of_version_or_publication_order(self):
        self.add_release(1, {"app.bin": b"old"})
        self.add_release(2, {"app.bin": b"new"})
        catalog = self.build(latest_id=1)
        self.assertEqual(catalog["latest_stable"], "v1.0.0")
        self.assertEqual(catalog["releases"][0]["tag"], "v2.0.0")

    def test_empty_catalog(self):
        self.assertEqual(self.build(), {"schema_version": 1, "latest_stable": None, "releases": []})
        self.assertIn("No releases published yet", (self.output / "index.html").read_text())

    def test_prerelease_only_does_not_become_latest_stable(self):
        self.add_release(prerelease=True)
        self.assertIsNone(self.build(latest_id=1)["latest_stable"])

    def test_missing_manifest_does_not_guess_firmware_role(self):
        self.add_release(files={"ESP32.bin": b"firmware"})
        self.assertIsNone(self.build()["releases"][0]["assets"][0]["role"])

    def test_paginates_releases(self):
        for release_id in range(1, 102):
            self.add_release(release_id)
        self.assertEqual(len(self.build()["releases"]), 101)
        self.assertIn(self.client.api_root + "/releases?per_page=100&page=2", [r.full_url for r in self.http.requests])

    def test_paginates_assets_independently_of_embedded_release_assets(self):
        files = {f"firmware-{index}.bin": bytes([index]) for index in range(101)}
        release, _ = self.add_release(files=files)
        release["assets"] = []
        self.assertEqual(len(self.build()["releases"][0]["assets"]), 101)

    def test_urls_and_html_escape_unusual_names_and_tags(self):
        name = 'firmware + # % " café.bin'
        self.add_release(files={name: b"app"}, tag_name="preview/next", name="<script>alert('x')</script>")
        asset = self.build()["releases"][0]["assets"][0]
        self.assertEqual(asset["url"], f"{BASE_URL}/releases/1/{quote(name, safe='')}")
        self.assertEqual((self.output / "releases/1" / name).read_bytes(), b"app")
        page = (self.output / "index.html").read_text()
        self.assertNotIn("<script>", page)
        self.assertIn("&lt;script&gt;", page)

    def test_api_token_is_not_sent_to_asset_downloads(self):
        self.add_release(files={"app.bin": b"app"})
        self.build()
        for request in self.http.requests:
            if urlsplit(request.full_url).hostname == "api.github.com":
                self.assertEqual(request.get_header("Authorization"), "Bearer fixture-token")
            else:
                self.assertIsNone(request.get_header("Authorization"))

    def test_redirect_strips_api_token_on_different_host(self):
        request = Request("https://api.github.com/example", headers={"Authorization": "Bearer fixture-token"})
        redirected = SafeRedirectHandler().redirect_request(request, None, 302, "", {}, "https://downloads.example/file")
        self.assertIsNone(redirected.get_header("Authorization"))

    def test_github_digest_mismatch_prevents_output(self):
        _, assets = self.add_release(files={"app.bin": b"app"})
        assets[0]["digest"] = "sha256:" + "0" * 64
        self.http.set_pages("/releases/1/assets", assets)
        with self.assertRaisesRegex(CatalogError, "SHA-256 mismatch.*GitHub"):
            self.build()
        self.assertFalse(self.output.exists())

    def test_manifest_hash_mismatch_prevents_output(self):
        files = self.firmware_files()
        files[next(iter(files))] = b"changed application"
        self.add_release(files=files)
        with self.assertRaisesRegex(CatalogError, "SHA-256 mismatch.*manifest.json"):
            self.build()
        self.assertFalse(self.output.exists())

    def test_checksum_file_mismatch_prevents_output(self):
        self.add_release(files={"app.bin": b"app", "SHA256SUMS": ("0" * 64 + "  app.bin\n").encode()})
        with self.assertRaisesRegex(CatalogError, "SHA-256 mismatch.*SHA256SUMS"):
            self.build()

    def test_missing_file_referenced_by_manifest_prevents_output(self):
        self.add_release(files={"manifest.json": json_bytes({"artifacts": [{"file": "absent.bin", "role": "app"}]})})
        with self.assertRaisesRegex(CatalogError, "missing release asset"):
            self.build()

    def test_malformed_manifest_prevents_output(self):
        self.add_release(files={"manifest.json": b"{invalid"})
        with self.assertRaisesRegex(CatalogError, "invalid manifest.json"):
            self.build()

    def test_missing_github_digest_still_computes_checksum(self):
        _, assets = self.add_release(files={"app.bin": b"app"})
        assets[0]["digest"] = None
        self.http.set_pages("/releases/1/assets", assets)
        self.assertEqual(self.build()["releases"][0]["assets"][0]["sha256"], sha256(b"app"))

    def test_download_failure_prevents_output(self):
        _, assets = self.add_release(files={"app.bin": b"app"})
        self.http.responses[assets[0]["browser_download_url"]] = OSError("connection interrupted")
        with self.assertRaisesRegex(CatalogError, "could not download"):
            self.build()
        self.assertFalse(self.output.exists())

    def test_short_download_prevents_output(self):
        _, assets = self.add_release(files={"app.bin": b"app"})
        self.http.responses[assets[0]["browser_download_url"]] = b"a"
        with self.assertRaisesRegex(CatalogError, "download size mismatch"):
            self.build()

    def test_api_failure_is_not_treated_as_empty_catalog(self):
        self.http.responses[self.client.api_root + "/releases?per_page=100&page=1"] = HTTPError("fixture", 403, "Forbidden", {}, None)
        with self.assertRaisesRegex(CatalogError, "HTTP 403"):
            generate_site(self.client, self.output, BASE_URL)
        self.assertFalse(self.output.exists())

    def test_latest_api_failure_other_than_404_prevents_output(self):
        self.http.responses[self.client.api_root + "/releases/latest"] = HTTPError("fixture", 500, "Error", {}, None)
        with self.assertRaisesRegex(CatalogError, "HTTP 500"):
            self.build()

    def test_declared_size_limit_is_checked_before_download(self):
        self.add_release(files={"app.bin": b"app"})
        with self.assertRaisesRegex(CatalogError, "1 GB limit"):
            self.build(max_bytes=2)
        self.assertFalse(any(r.full_url.startswith("https://downloads.example/") for r in self.http.requests))

    def test_actual_download_size_is_bounded(self):
        _, assets = self.add_release(files={"app.bin": b"app"})
        with self.assertRaisesRegex(CatalogError, "1 GB limit"):
            self.client.download(assets[0], self.output, max_bytes=2)

    def test_size_limit_includes_generated_documents(self):
        with self.assertRaisesRegex(CatalogError, "1 GB limit"):
            self.build(max_bytes=100)
        self.assertFalse(self.output.exists())

    def test_rejects_path_traversal(self):
        self.add_release(files={"../outside.bin": b"app"})
        with self.assertRaisesRegex(CatalogError, "unsafe asset filename"):
            self.build()
        self.assertFalse((self.output.parent / "outside.bin").exists())

    def test_rejects_incomplete_upload(self):
        _, assets = self.add_release(files={"app.bin": b"app"})
        assets[0]["state"] = "starter"
        self.http.set_pages("/releases/1/assets", assets)
        with self.assertRaisesRegex(CatalogError, "not fully uploaded"):
            self.build()

    def test_existing_output_is_preserved(self):
        self.output.mkdir()
        existing = self.output / "index.html"
        existing.write_text("existing site")
        with self.assertRaisesRegex(CatalogError, "new or empty"):
            self.build()
        self.assertEqual(existing.read_text(), "existing site")

    def test_accepts_empty_output_directory(self):
        self.output.mkdir()
        self.build()
        self.assertTrue((self.output / "index.html").is_file())

    def test_complete_rebuild_omits_removed_releases(self):
        self.add_release(files={"old.bin": b"old"})
        self.build(latest_id=1)
        old_output = self.output
        self.output = self.output.parent / "next-site"
        self.releases.clear()
        self.http.responses.pop(self.client.api_root + "/releases/latest")
        self.assertEqual(self.build()["releases"], [])
        self.assertFalse((self.output / "releases/1").exists())
        self.assertTrue((old_output / "releases/1/old.bin").is_file())


if __name__ == "__main__":
    unittest.main()
