#!/usr/bin/env python3
"""Mirror published GitHub release assets into a static firmware catalog."""

from __future__ import annotations

import argparse
import hashlib
import html
import json
import os
from pathlib import Path
import re
from string import Template
import sys
import tempfile
from typing import Any
from urllib.error import HTTPError, URLError
from urllib.parse import quote, urlsplit
from urllib.request import HTTPRedirectHandler, Request, build_opener


MAX_SITE_BYTES = 1_000_000_000
TEMPLATE = Path(__file__).with_name("firmware_pages.html")


class CatalogError(Exception):
    """An incomplete or inconsistent mirror must not be deployed."""


class SafeRedirectHandler(HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        if urlsplit(newurl).scheme != "https":
            raise CatalogError("GitHub redirected to a non-HTTPS URL")
        redirected = super().redirect_request(req, fp, code, msg, headers, newurl)
        if redirected is not None and urlsplit(req.full_url).netloc != urlsplit(newurl).netloc:
            redirected.remove_header("Authorization")
        return redirected


class GitHubClient:
    def __init__(self, repository: str, token: str | None = None):
        if not re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", repository):
            raise CatalogError("repository must be OWNER/REPO")
        self.repository = repository
        self.api_root = f"https://api.github.com/repos/{repository}"
        self.token = token
        self.opener = build_opener(SafeRedirectHandler())

    def request_json(self, path: str, *, missing_ok: bool = False) -> Any:
        headers = {
            "Accept": "application/vnd.github+json",
            "User-Agent": "thymio3-firmware-pages",
            "X-GitHub-Api-Version": "2022-11-28",
        }
        if self.token:
            headers["Authorization"] = f"Bearer {self.token}"
        request = Request(self.api_root + path, headers=headers)
        try:
            with self.opener.open(request, timeout=60) as response:
                return json.load(response)
        except HTTPError as exc:
            exc.close()
            if missing_ok and exc.code == 404:
                return None
            raise CatalogError(f"GitHub API {path}: HTTP {exc.code}") from exc
        except (URLError, OSError, ValueError) as exc:
            raise CatalogError(f"could not read GitHub API {path}: {exc}") from exc

    def paginate(self, path: str):
        page = 1
        while True:
            entries = self.request_json(f"{path}?per_page=100&page={page}")
            if not isinstance(entries, list):
                raise CatalogError(f"expected a list from GitHub API {path}")
            yield from entries
            if len(entries) < 100:
                return
            page += 1

    def download(self, asset: dict, destination: Path, max_bytes: int) -> tuple[int, str]:
        # Public download redirects must never receive the API token.
        url = asset["browser_download_url"]
        if urlsplit(url).scheme != "https":
            raise CatalogError(f"asset URL must use HTTPS: {asset['name']}")
        request = Request(url, headers={"User-Agent": "thymio3-firmware-pages"})
        size = 0
        digest = hashlib.sha256()
        try:
            with self.opener.open(request, timeout=60) as response, destination.open("xb") as output:
                while chunk := response.read(1024 * 1024):
                    size += len(chunk)
                    if size > max_bytes:
                        raise CatalogError("site exceeds the GitHub Pages 1 GB limit")
                    output.write(chunk)
                    digest.update(chunk)
        except HTTPError as exc:
            exc.close()
            raise CatalogError(f"could not download {asset['name']}: HTTP {exc.code}") from exc
        except (URLError, OSError) as exc:
            raise CatalogError(f"could not download {asset['name']}: {exc}") from exc
        return size, digest.hexdigest()


def asset_name(value: str) -> str:
    if (
        not isinstance(value, str)
        or value in ("", ".", "..")
        or any(char in value for char in "/\\")
        or any(ord(char) < 32 or ord(char) == 127 for char in value)
    ):
        raise CatalogError(f"unsafe asset filename: {value!r}")
    return value


def verify_hash(asset: dict, expected: str, source: str) -> None:
    if not isinstance(expected, str) or not re.fullmatch(r"[0-9a-fA-F]{64}", expected):
        raise CatalogError(f"invalid SHA-256 for {asset['name']} in {source}")
    if asset["sha256"] != expected.lower():
        raise CatalogError(f"SHA-256 mismatch for {asset['name']} in {source}")


def apply_metadata(directory: Path, assets: list[dict]) -> None:
    by_name = {asset["name"]: asset for asset in assets}

    def find(name: str, source: str) -> dict:
        name = asset_name(name)
        if name not in by_name:
            raise CatalogError(f"{source} references a missing release asset: {name}")
        return by_name[name]

    if "manifest.json" in by_name:
        try:
            manifest = json.loads((directory / "manifest.json").read_text(encoding="utf-8"))
            entries = list(manifest.get("artifacts", []))
            if manifest.get("merged") is not None:
                entries.append({**manifest["merged"], "role": "full"})
            for entry in entries:
                asset = find(entry["file"], "manifest.json")
                role = entry["role"]
                if not isinstance(role, str) or not role:
                    raise CatalogError("manifest.json contains an invalid artifact role")
                if asset["role"] is not None and asset["role"] != role:
                    raise CatalogError(f"conflicting roles for {asset['name']}")
                asset["role"] = role
                if "sha256" in entry:
                    verify_hash(asset, entry["sha256"], "manifest.json")
                if "size" in entry and asset["size"] != entry["size"]:
                    raise CatalogError(f"size mismatch for {asset['name']} in manifest.json")
        except (ValueError, KeyError, TypeError, AttributeError) as exc:
            raise CatalogError(f"invalid manifest.json: {exc}") from exc

    if "SHA256SUMS" in by_name:
        for line in (directory / "SHA256SUMS").read_text(encoding="utf-8").splitlines():
            if not line.strip():
                continue
            match = re.fullmatch(r"([0-9a-fA-F]{64}) [ *](.+)", line)
            if not match:
                raise CatalogError("invalid SHA256SUMS entry")
            expected, name = match.groups()
            verify_hash(find(name, "SHA256SUMS"), expected, "SHA256SUMS")


def render_html(catalog: dict, repository: str) -> str:
    escape = html.escape
    sections = []
    latest_link = '<p class="latest">No stable release is currently published.</p>'
    labels = {"app": "Application firmware · Bluetooth update", "full": "Full flash image"}
    for release in catalog["releases"]:
        latest = release["tag"] == catalog["latest_stable"]
        anchor = f"release-{release['id']}"
        badges = ""
        if latest:
            badges += '<span class="badge stable">Latest stable</span>'
            latest_link = (
                f'<p class="latest">Latest stable: <a href="#{anchor}">'
                f'{escape(release["tag"])}</a></p>'
            )
        if release["prerelease"]:
            badges += '<span class="badge preview">Prerelease</span>'
        rows = []
        for asset in release["assets"]:
            label = labels.get(asset["role"], asset["role"] or "Release file")
            css_class = ' class="application"' if asset["role"] == "app" else ""
            rows.append(
                f'<li{css_class}><div class="asset-heading"><div>'
                f'<span class="role">{escape(label)}</span>'
                f'<a class="filename" href="{escape(asset["url"], quote=True)}" '
                f'download>{escape(asset["name"])}</a></div>'
                f'<span class="size">{asset["size"]:,} bytes</span></div>'
                f'<details><summary>SHA-256 checksum</summary>'
                f'<code>{asset["sha256"]}</code></details></li>'
            )
        files = '<ul class="assets">' + "".join(rows) + "</ul>" if rows else "<p>No attached files.</p>"
        sections.append(
            f'<section class="release" id="{anchor}">'
            f'<div class="release-heading"><h2>{escape(release["name"])}</h2>{badges}</div>'
            f'<p class="release-meta">{escape(release["tag"])} · '
            f'<time datetime="{escape(release["published_at"], quote=True)}">'
            f'{escape(release["published_at"][:10])}</time> · '
            f'<a href="{escape(release["release_url"], quote=True)}">Release on GitHub</a></p>'
            f'{files}</section>'
        )
    return Template(TEMPLATE.read_text(encoding="utf-8")).substitute(
        repository_url=escape(f"https://github.com/{repository}", quote=True),
        latest_link=latest_link,
        release_sections="\n".join(sections) or '<p class="empty">No releases published yet.</p>',
    )


def generate_site(
    client: GitHubClient,
    output_dir: Path,
    base_url: str,
    *,
    max_bytes: int = MAX_SITE_BYTES,
) -> dict:
    parsed_base = urlsplit(base_url)
    if parsed_base.scheme != "https" or not parsed_base.netloc or parsed_base.query or parsed_base.fragment:
        raise CatalogError("base URL must be an absolute HTTPS URL without a query or fragment")
    base_url = base_url.rstrip("/")
    output_dir = Path(output_dir)
    if output_dir.exists() and (not output_dir.is_dir() or any(output_dir.iterdir())):
        raise CatalogError("output directory must be new or empty")

    releases = [release for release in client.paginate("/releases") if not release["draft"]]
    releases.sort(key=lambda release: (release["published_at"], release["id"]), reverse=True)
    latest = client.request_json("/releases/latest", missing_ok=True)
    latest_id = latest["id"] if latest else None
    catalog = {"schema_version": 1, "latest_stable": None, "releases": []}
    total_size = 0
    output_dir.parent.mkdir(parents=True, exist_ok=True)
    # A failed build never leaves a partial catalog in the requested output directory.
    with tempfile.TemporaryDirectory(prefix="firmware-pages-", dir=output_dir.parent) as temporary:
        staging = Path(temporary) / "site"
        staging.mkdir()
        seen_ids = set()
        for release in releases:
            release_id = release["id"]
            if not isinstance(release_id, int) or release_id <= 0 or release_id in seen_ids:
                raise CatalogError("invalid or duplicate release ID")
            seen_ids.add(release_id)
            relative_dir = f"releases/{release_id}"
            directory = staging / relative_dir
            directory.mkdir(parents=True)
            assets = []
            seen_names = set()
            for source in client.paginate(f"/releases/{release_id}/assets"):
                name = asset_name(source["name"])
                if name in seen_names:
                    raise CatalogError(f"duplicate release asset: {name}")
                seen_names.add(name)
                if source["state"] != "uploaded":
                    raise CatalogError(f"release asset is not fully uploaded: {name}")
                if total_size + source["size"] > max_bytes:
                    raise CatalogError("site exceeds the GitHub Pages 1 GB limit")
                size, digest = client.download(source, directory / name, max_bytes - total_size)
                if size != source["size"]:
                    raise CatalogError(f"download size mismatch for {name}")
                total_size += size
                asset = {
                    "name": name,
                    "url": f"{base_url}/{relative_dir}/{quote(name, safe='')}",
                    "size": size,
                    "sha256": digest,
                    "role": None,
                }
                if source.get("digest", "") and source["digest"].startswith("sha256:"):
                    verify_hash(asset, source["digest"][7:], "GitHub asset metadata")
                assets.append(asset)
            apply_metadata(directory, assets)
            assets.sort(key=lambda asset: (asset["role"] != "app", asset["name"]))
            if release_id == latest_id and not release["prerelease"]:
                catalog["latest_stable"] = release["tag_name"]
            catalog["releases"].append({
                "id": release_id,
                "tag": release["tag_name"],
                "name": release["name"] or release["tag_name"],
                "published_at": release["published_at"],
                "prerelease": release["prerelease"],
                "release_url": release["html_url"],
                "assets": assets,
            })

        documents = {
            "releases.json": json.dumps(catalog, indent=2) + "\n",
            "index.html": render_html(catalog, client.repository),
            ".nojekyll": "",
        }
        for name, content in documents.items():
            encoded = content.encode("utf-8")
            total_size += len(encoded)
            if total_size > max_bytes:
                raise CatalogError("site exceeds the GitHub Pages 1 GB limit")
            (staging / name).write_bytes(encoded)
        if output_dir.exists():
            output_dir.rmdir()  # Only an empty directory is accepted above.
        staging.rename(output_dir)
    return catalog


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repository", default=os.environ.get("GITHUB_REPOSITORY", "Mobsya/thymio3-firmware-esp32"))
    parser.add_argument("--base-url", required=True, help="Public GitHub Pages site URL.")
    parser.add_argument("--output-dir", type=Path, default=Path("_site"), help="A new or empty directory.")
    args = parser.parse_args()
    try:
        client = GitHubClient(args.repository, os.environ.get("GH_TOKEN") or os.environ.get("GITHUB_TOKEN"))
        catalog = generate_site(client, args.output_dir, args.base_url)
    except (CatalogError, OSError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    count = sum(len(release["assets"]) for release in catalog["releases"])
    print(f"Mirrored {len(catalog['releases'])} releases and {count} assets into {args.output_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
