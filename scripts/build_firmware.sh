#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd)"

: "${ADF_REPO:=https://github.com/Mobsya/esp-adf.git}"
: "${ADF_REF:=release/v2.4}"
: "${ADF_PATH:=${REPO_ROOT}/.deps/esp-adf_release2.4}"
: "${IDF_TARGET:=esp32}"
: "${BUILD_DIR:=build}"

abs_path() {
    case "$1" in
        /*) printf '%s\n' "$1" ;;
        *) printf '%s\n' "${REPO_ROOT}/$1" ;;
    esac
}

ADF_PATH="$(abs_path "$ADF_PATH")"
BUILD_DIR="$(abs_path "$BUILD_DIR")"
export ADF_REPO ADF_REF ADF_PATH IDF_TARGET BUILD_DIR

"${SCRIPT_DIR}/setup_firmware_env.sh"

PYTHON_SHIM_DIR="${REPO_ROOT}/.deps/python-bin"
if [ -d "$PYTHON_SHIM_DIR" ]; then
    export PATH="${PYTHON_SHIM_DIR}:$PATH"
fi

printf 'Initializing firmware submodules\n'
git -C "$REPO_ROOT" submodule update --init --recursive

MICROPY_DIR="${REPO_ROOT}/components/mp_component/micropython"
if [ ! -d "$MICROPY_DIR" ]; then
    printf 'error: MicroPython submodule not found at %s\n' "$MICROPY_DIR" >&2
    exit 1
fi

IDF_EXPORT="${ADF_PATH}/esp-idf/export.sh"
if [ ! -f "$IDF_EXPORT" ]; then
    printf 'error: ESP-IDF export script not found at %s\n' "$IDF_EXPORT" >&2
    exit 1
fi

printf 'Loading ESP-IDF environment from %s\n' "$IDF_EXPORT"
set +u
# shellcheck source=/dev/null
source "$IDF_EXPORT"
set -u

printf 'Building MicroPython cross compiler\n'
make -C "${MICROPY_DIR}/mpy-cross"

printf 'Updating MicroPython ESP32 port submodules\n'
make -C "${MICROPY_DIR}/ports/esp32" submodules

printf 'Building firmware for %s\n' "$IDF_TARGET"
cd "$REPO_ROOT"
idf.py -B "$BUILD_DIR" -DIDF_TARGET="$IDF_TARGET" build
