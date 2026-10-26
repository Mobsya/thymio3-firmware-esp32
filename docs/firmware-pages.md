# Browser firmware downloads (GitHub Pages)

The `Firmware Pages` workflow mirrors published GitHub Release assets to:

- Download page: <https://mobsya.github.io/thymio3-firmware-esp32/>
- JSON catalog: <https://mobsya.github.io/thymio3-firmware-esp32/releases.json>

The binary files themselves are served by Pages, so another website can fetch
them as an `ArrayBuffer`. Links to GitHub Release downloads would still be
subject to GitHub's release-download CORS policy. The catalog and its downloads
are public and require no authentication.

## Initial setup

Once the Pages implementation is on `master`, a repository administrator must:

1. Open **Settings → Pages** and select **GitHub Actions** as the build source.
2. Open **Settings → Environments → github-pages** (create the environment if
   needed). Under deployment branches and tags, allow the `master` branch and
   release tags matching `v*`. Add other tag patterns if releases use them.
   Release-event runs use tag refs even though the generator checks out `master`.
3. Run **Actions → Firmware Pages → Run workflow**, selecting `master`, to
   backfill all existing published releases.
4. Wait for the deployment job, then open the page and test a browser download
   using the example below from a different origin.

The workflow gets the public base URL from GitHub's Pages configuration, so the
catalog also works with a configured custom domain or a fork's Pages URL.

## Automatic updates

After a successful `Firmware Build`, a `workflow_run` event starts the mirror.
This handles both new releases and replacement assets uploaded by the existing
build workflow: release changes made with `GITHUB_TOKEN` do not trigger ordinary
release-event workflows. Failed firmware builds do not start a deployment.

The mirror also runs for release publication, edits, deletion, unpublication,
and stable/prerelease transitions, and when its implementation changes on
`master`. Manually adding, replacing, or deleting an individual asset may not
produce a release event; run **Firmware Pages** manually after such changes.
For edits to old tags that predate the Pages workflow, use a manual run as well.

Every run fetches the complete published release history, including prereleases
and all uploaded assets. Drafts and GitHub-generated source archives are excluded.
Files are copied without modification to `releases/<release-id>/<filename>`.
The numeric release ID keeps paths independent of tag spelling. Deleting or
unpublishing a release removes it from the next successful deployment.

The generator reads the application/component roles from each release's
`manifest.json`, checks sizes and available SHA-256 hashes from the GitHub API,
manifest, and `SHA256SUMS`, and computes a hash for every mirrored file. A release
without a manifest is still listed, but its file roles are `null`; filenames are
never used to guess which binary is suitable for a Bluetooth update.

Runs are serialized. Download errors, incomplete uploads, inconsistent metadata,
checksum mismatches, and exceeding the 1 GB Pages site limit stop publication.
The previously deployed site remains available; inspect the failed Actions run
and rerun after correcting the release. History is never silently truncated.
Generated site files and firmware binaries are deployment artifacts, not Git
commits. The firmware compilation and packaging workflow is unchanged.

## Catalog interface

`releases.json` has `schema_version: 1`, `latest_stable`, and a `releases` array
ordered by publication date, newest first. `latest_stable` is the tag selected
by GitHub's latest-release endpoint, or `null` when there is no published stable
release. It never falls back to a prerelease.

Each release provides `id`, `tag`, `name`, `published_at` (UTC timestamp),
`prerelease`, `release_url` (the original GitHub Release), and `assets`.
Each asset provides `name`, an absolute Pages `url`, `size` in bytes, `sha256`
(lowercase hex), and `role`. Manifest roles such as `app`, `bootloader`,
`partition-table`, `ota-data`, and `audio-tone` are preserved. The merged image's
role is `full`; unclassified files use `null`.

Use `role === 'app'` for the application firmware. The full-flash image includes
other flash regions and is a separate download. The catalog supplies downloads;
the existing browser library remains responsible for Bluetooth flashing.

```js
const catalogUrl =
  'https://mobsya.github.io/thymio3-firmware-esp32/releases.json';

async function downloadLatestFirmware() {
  const catalogResponse = await fetch(catalogUrl, {
    credentials: 'omit',
    cache: 'no-cache',
  });
  if (!catalogResponse.ok) {
    throw new Error(`Catalog download failed: HTTP ${catalogResponse.status}`);
  }
  const catalog = await catalogResponse.json();
  if (catalog.schema_version !== 1) throw new Error('Unsupported catalog version');
  const release = catalog.releases.find(
    entry => entry.tag === catalog.latest_stable && !entry.prerelease,
  );
  const firmware = release?.assets.find(asset => asset.role === 'app');
  if (!firmware) throw new Error('No stable application firmware is available');

  const response = await fetch(firmware.url, {
    credentials: 'omit',
    cache: 'no-cache',
  });
  if (!response.ok) throw new Error(`Firmware download failed: HTTP ${response.status}`);
  const buffer = await response.arrayBuffer();
  if (buffer.byteLength !== firmware.size) throw new Error('Firmware size mismatch');
  const digest = await crypto.subtle.digest('SHA-256', buffer);
  const checksum = Array.from(new Uint8Array(digest), byte =>
    byte.toString(16).padStart(2, '0'),
  ).join('');
  if (checksum !== firmware.sha256) throw new Error('Firmware checksum mismatch');
  return buffer; // Pass this ArrayBuffer to your Bluetooth flashing library.
}
```

Run this from an HTTPS page or localhost. Both fetches are ordinary CORS GET
requests with no custom headers. Do not use `mode: 'no-cors'`: an opaque response
does not expose the binary data. The size and digest checks also detect stale
or inconsistent downloads while a deployment propagates through the Pages CDN.

## Local validation

The generator uses the Python standard library (Python 3.10 or newer). To build
a local copy without compiling firmware, run these commands from the repository
root using a new or empty output directory:

```sh
python3 -m unittest discover -s scripts/tests -p 'test_firmware_pages.py' -v
python3 scripts/build_firmware_pages.py \
  --repository Mobsya/thymio3-firmware-esp32 \
  --base-url https://mobsya.github.io/thymio3-firmware-esp32 \
  --output-dir _site
python3 -m http.server 8000 --directory _site
```

For another generation, choose another empty `--output-dir`; the generator
refuses to overwrite existing files. `_site/` is ignored by Git. The local
server previews the catalog; its absolute download URLs still point to Pages.
Optional `GH_TOKEN` or `GITHUB_TOKEN` environment variables authenticate GitHub
API requests to increase the rate limit. Tokens are never sent to public asset
downloads or written to the site.
