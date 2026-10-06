#!/usr/bin/env python3
from __future__ import annotations

import argparse
import ast
import hashlib
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


REPO_ROOT = Path(__file__).resolve().parents[1]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Package ESP-IDF firmware build artifacts.")
    parser.add_argument("--build-dir", default=os.environ.get("BUILD_DIR", "build"))
    parser.add_argument("--dist-dir", default=os.environ.get("DIST_DIR", "dist"))
    parser.add_argument(
        "--project-name",
        default=os.environ.get("FIRMWARE_PROJECT_NAME", "thymio3-esp32-firmware"),
    )
    parser.add_argument("--idf-target", default=os.environ.get("IDF_TARGET", "esp32"))
    parser.add_argument(
        "--version-file",
        default=os.environ.get("FIRMWARE_VERSION_FILE", "main/common.h"),
        help="Header containing FIRMWARE_VERSION_* defines.",
    )
    parser.add_argument(
        "--artifact-prefix",
        default=os.environ.get("ARTIFACT_PREFIX", "ESP32"),
        help="Prefix used in release artifact file names.",
    )
    parser.add_argument(
        "--release-date",
        default=os.environ.get("RELEASE_DATE"),
        help="Release date for artifact names, in YYYY-MM-DD format. Defaults to today in UTC.",
    )
    parser.add_argument(
        "--commit-hash",
        default=os.environ.get("COMMIT_HASH"),
        help="Commit hash for artifact names. Defaults to the current Git commit.",
    )
    return parser.parse_args()


def resolve_path(value: str, base: Path = REPO_ROOT) -> Path:
    path = Path(value)
    if path.is_absolute():
        return path
    return (base / path).resolve()


def fail(message: str) -> None:
    print(f"error: {message}", file=sys.stderr)
    raise SystemExit(1)


def load_json(path: Path) -> dict[str, Any]:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError:
        fail(f"required build metadata not found: {path}")
    except json.JSONDecodeError as exc:
        fail(f"could not parse {path}: {exc}")


def git_value(*args: str) -> str | None:
    result = subprocess.run(
        ["git", "-C", str(REPO_ROOT), *args],
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        text=True,
    )
    value = result.stdout.strip()
    return value or None


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def relative_or_absolute(path: Path) -> str:
    try:
        return str(path.relative_to(REPO_ROOT))
    except ValueError:
        return str(path)


def strip_line_comment(value: str) -> str:
    return value.split("//", 1)[0].strip()


def parse_define_value(raw_value: str) -> str:
    value = strip_line_comment(raw_value)
    token = value.split(None, 1)[0]

    if token.startswith('"'):
        try:
            parsed = ast.literal_eval(token)
        except (SyntaxError, ValueError):
            fail(f"could not parse firmware version string: {token}")
        return str(parsed)

    return token


def read_firmware_version(version_file: Path) -> str:
    if not version_file.exists():
        fail(f"firmware version header not found: {version_file}")

    defines: dict[str, str] = {}
    define_re = re.compile(r"^\s*#\s*define\s+(FIRMWARE_VERSION_(?:MAJOR|MINOR|PATCH))\s+(.+?)\s*$")

    for line in version_file.read_text(encoding="utf-8").splitlines():
        match = define_re.match(line)
        if match:
            defines[match.group(1)] = parse_define_value(match.group(2))

    required = ["FIRMWARE_VERSION_MAJOR", "FIRMWARE_VERSION_MINOR", "FIRMWARE_VERSION_PATCH"]
    missing = [name for name in required if name not in defines]
    if missing:
        fail(f"missing firmware version define(s) in {version_file}: {', '.join(missing)}")

    version = ".".join(defines[name] for name in required)
    return sanitize_component(version, "firmware version")


def release_date(value: str | None) -> str:
    if value is None or value == "":
        return datetime.now(timezone.utc).strftime("%Y-%m-%d")

    if not re.fullmatch(r"\d{4}-\d{2}-\d{2}", value):
        fail(f"release date must use YYYY-MM-DD format: {value}")

    return value


def commit_hash(value: str | None) -> str:
    if value is None or value == "":
        value = os.environ.get("GITHUB_SHA") or git_value("rev-parse", "--short=7", "HEAD")

    if value is None or value == "":
        fail("could not determine commit hash for artifact name")

    if re.fullmatch(r"[0-9a-fA-F]{8,}", value):
        value = value[:7]

    return sanitize_component(value, "commit hash")


def sanitize_component(value: str, label: str) -> str:
    sanitized = re.sub(r"[^A-Za-z0-9._-]+", "-", value).strip("-")
    if not sanitized:
        fail(f"{label} cannot be empty after sanitization")
    return sanitized


def artifact_role(project_name: str, source: Path, offset: str) -> str:
    lower_path = str(source).lower()
    lower_name = source.name.lower()
    lower_project = f"{project_name}.bin".lower()
    lower_offset = offset.lower()

    if "bootloader" in lower_path:
        return "bootloader"
    if "partition" in lower_path:
        return "partition-table"
    if "ota" in lower_name and "data" in lower_name:
        return "ota-data"
    if lower_name == lower_project or lower_offset == "0x10000":
        return "app"

    stem = re.sub(r"[^a-z0-9]+", "-", source.stem.lower()).strip("-")
    return stem or f"offset-{lower_offset.replace('0x', '')}"


def artifact_name(release_basename: str, role: str, offset: str, used: set[str]) -> str:
    if role == "app":
        base = f"{release_basename}.bin"
    else:
        base = f"{release_basename}-{role}.bin"

    if base not in used:
        used.add(base)
        return base

    suffix = offset.lower().replace("0x", "").replace("/", "-")
    name = f"{release_basename}-{role}-{suffix}.bin"
    used.add(name)
    return name


def flash_entries(flasher_args: dict[str, Any]) -> list[tuple[str, str]]:
    flash_files = flasher_args.get("flash_files")
    entries: list[tuple[str, str]] = []

    if isinstance(flash_files, dict):
        entries.extend((str(offset), str(file_name)) for offset, file_name in flash_files.items())

    if not entries:
        for key in ("bootloader", "partition_table", "partition-table", "ota_data", "app", "project"):
            item = flasher_args.get(key)
            if isinstance(item, dict) and "offset" in item and "file" in item:
                entries.append((str(item["offset"]), str(item["file"])))

    if not entries:
        fail("flasher_args.json does not contain flash file entries")

    return sorted(entries, key=lambda item: int(item[0], 0))


def resolve_flash_file(build_dir: Path, file_name: str) -> Path:
    candidate = Path(file_name)
    candidates: list[Path] = []

    if candidate.is_absolute():
        candidates.append(candidate)
    else:
        candidates.extend([build_dir / candidate, REPO_ROOT / candidate, candidate])

    for path in candidates:
        if path.exists():
            return path.resolve()

    fail(f"flash binary listed in flasher_args.json was not found: {file_name}")


def write_flash_args(path: Path, write_flash_args: list[str], entries: list[dict[str, Any]]) -> None:
    args = [str(arg) for arg in write_flash_args]
    for entry in entries:
        args.extend([entry["offset"], entry["file"]])
    path.write_text("\n".join(args) + "\n", encoding="utf-8")


def build_flash_args_from_json(
    build_dir: Path,
    flasher_args: dict[str, Any],
    entries: list[tuple[str, str]],
) -> Path:
    generated = tempfile.NamedTemporaryFile(
        mode="w",
        encoding="utf-8",
        prefix="generated-flash-args-",
        dir=build_dir,
        delete=False,
    )
    with generated:
        for arg in flasher_args.get("write_flash_args", []):
            generated.write(f"{arg}\n")
        for offset, file_name in entries:
            source = resolve_flash_file(build_dir, file_name)
            generated.write(f"{offset}\n")
            generated.write(f"{source}\n")
    return Path(generated.name)


def run_merge_bin(build_dir: Path, merged_path: Path, idf_target: str, flash_args_path: Path) -> str:
    adf_path = resolve_path(os.environ.get("ADF_PATH", ".deps/esp-adf_release2.4"))
    idf_export = adf_path / "esp-idf" / "export.sh"
    esptool_py = adf_path / "esp-idf" / "components" / "esptool_py" / "esptool" / "esptool.py"
    env = os.environ.copy()
    python_shim_dir = REPO_ROOT / ".deps" / "python-bin"
    if python_shim_dir.exists():
        env["PATH"] = f"{python_shim_dir}{os.pathsep}{env.get('PATH', '')}"

    if idf_export.exists():
        source_prefix = f"source {shlex.quote(str(idf_export))} >/dev/null"
        python_command = "python"
    else:
        source_prefix = ":"
        python_command = "python3"

    if flash_args_path.parent == build_dir:
        flash_arg_ref = f"@{flash_args_path.name}"
    else:
        flash_arg_ref = f"@{flash_args_path}"

    command = (
        "set -e; "
        f"{source_prefix}; "
        f"cd {shlex.quote(str(build_dir))}; "
        f"{python_command} {shlex.quote(str(esptool_py))} --chip {shlex.quote(idf_target)} "
        f"merge_bin -o {shlex.quote(str(merged_path))} {shlex.quote(flash_arg_ref)}"
    )

    result = subprocess.run(["bash", "-c", command], check=False, env=env)
    if result.returncode != 0:
        fail(
            "could not generate merged firmware image with esptool; "
            "run make setup first or source the ESP-IDF environment"
        )

    return "esptool.py merge_bin"


def checksums_for(dist_dir: Path) -> list[tuple[str, str]]:
    items = []
    for path in sorted(dist_dir.iterdir()):
        if path.is_file() and path.name != "SHA256SUMS":
            items.append((sha256(path), path.name))
    return items


def main() -> None:
    args = parse_args()
    build_dir = resolve_path(args.build_dir)
    dist_dir = resolve_path(args.dist_dir)
    project_name = args.project_name
    idf_target = args.idf_target
    version_file = resolve_path(args.version_file)
    firmware_version = read_firmware_version(version_file)
    release_date_value = release_date(args.release_date)
    commit_hash_value = commit_hash(args.commit_hash)
    artifact_prefix = sanitize_component(args.artifact_prefix, "artifact prefix")
    artifact_date = release_date_value.replace("-", "")
    release_basename = f"{artifact_prefix}-{artifact_date}-{commit_hash_value}-{firmware_version}"

    if not build_dir.exists():
        fail(f"build directory does not exist: {build_dir}")

    flasher_args_path = build_dir / "flasher_args.json"
    flasher_args = load_json(flasher_args_path)
    source_entries = flash_entries(flasher_args)
    write_args = [str(arg) for arg in flasher_args.get("write_flash_args", [])]

    if dist_dir.exists():
        shutil.rmtree(dist_dir)
    dist_dir.mkdir(parents=True)

    build_flash_args = build_dir / "flash_args"
    generated_flash_args: Path | None = None
    if build_flash_args.exists():
        merge_flash_args = build_flash_args
        merge_args_source = "build/flash_args"
    else:
        generated_flash_args = build_flash_args_from_json(build_dir, flasher_args, source_entries)
        merge_flash_args = generated_flash_args
        merge_args_source = "flasher_args.json"

    merged_name = f"FULL-{release_basename}.bin"
    merged_path = dist_dir / merged_name
    merge_tool = run_merge_bin(build_dir, merged_path, idf_target, merge_flash_args)

    packaged_entries: list[dict[str, Any]] = []
    used_names = {merged_name}
    for offset, file_name in source_entries:
        source = resolve_flash_file(build_dir, file_name)
        role = artifact_role(project_name, source, offset)
        dest_name = artifact_name(release_basename, role, offset, used_names)
        dest = dist_dir / dest_name
        shutil.copy2(source, dest)
        packaged_entries.append(
            {
                "role": role,
                "offset": offset,
                "file": dest_name,
                "source": relative_or_absolute(source),
                "size": dest.stat().st_size,
                "sha256": sha256(dest),
            }
        )

    shutil.copy2(flasher_args_path, dist_dir / "flasher_args.json")
    write_flash_args(dist_dir / "flash_args", write_args, packaged_entries)

    manifest = {
        "project": project_name,
        "firmware_version": firmware_version,
        "release_date": release_date_value,
        "commit_hash": commit_hash_value,
        "artifact_prefix": artifact_prefix,
        "release_basename": release_basename,
        "created_at": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"),
        "git": {
            "commit": os.environ.get("GITHUB_SHA") or git_value("rev-parse", "HEAD"),
            "ref": os.environ.get("GITHUB_REF_NAME") or git_value("branch", "--show-current"),
            "describe": git_value("describe", "--tags", "--always", "--dirty"),
        },
        "idf_target": idf_target,
        "adf": {
            "repo": os.environ.get("ADF_REPO", "https://github.com/Mobsya/esp-adf.git"),
            "ref": os.environ.get("ADF_REF", "release/v2.4"),
            "path": os.environ.get("ADF_PATH", ".deps/esp-adf_release2.4"),
        },
        "build": {
            "build_dir": relative_or_absolute(build_dir),
            "version_file": relative_or_absolute(version_file),
            "merge_args_source": merge_args_source,
            "merge_tool": merge_tool,
            "write_flash_args": write_args,
            "extra_esptool_args": flasher_args.get("extra_esptool_args", {}),
        },
        "artifacts": packaged_entries,
        "merged": {
            "offset": "0x0",
            "file": merged_name,
            "size": merged_path.stat().st_size,
            "sha256": sha256(merged_path),
        },
    }
    (dist_dir / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")

    sums = checksums_for(dist_dir)
    (dist_dir / "SHA256SUMS").write_text(
        "".join(f"{digest}  {file_name}\n" for digest, file_name in sums),
        encoding="utf-8",
    )

    if generated_flash_args is not None:
        generated_flash_args.unlink(missing_ok=True)

    print(f"Packaged firmware artifacts in {dist_dir}")


if __name__ == "__main__":
    main()
