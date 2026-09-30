#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd)"

: "${ADF_REPO:=https://github.com/Mobsya/esp-adf.git}"
: "${ADF_REF:=release/v2.4}"
: "${ADF_PATH:=${REPO_ROOT}/.deps/esp-adf_release2.4}"
: "${IDF_TARGET:=esp32}"
: "${IDF_BOOTSTRAP_PYTHON:=}"

# shellcheck source=firmware_git.sh
source "${SCRIPT_DIR}/firmware_git.sh"

abs_path() {
    case "$1" in
        /*) printf '%s\n' "$1" ;;
        *) printf '%s\n' "${REPO_ROOT}/$1" ;;
    esac
}

require_command() {
    if ! command -v "$1" >/dev/null 2>&1; then
        printf 'error: required command not found: %s\n' "$1" >&2
        exit 1
    fi
}

python_is_supported() {
    "$1" -c 'import sys; raise SystemExit(0 if (3, 6) <= sys.version_info[:2] < (3, 13) else 1)' >/dev/null 2>&1
}

python_version() {
    "$1" -c 'import sys; print(".".join(str(part) for part in sys.version_info[:3]))' 2>/dev/null
}

resolve_command() {
    case "$1" in
        */*) printf '%s\n' "$1" ;;
        *) command -v "$1" 2>/dev/null || true ;;
    esac
}

find_pyenv_python() {
    command -v pyenv >/dev/null 2>&1 || return 1

    local version
    for prefix in 3.12 3.11 3.10 3.9 3.8; do
        version="$(pyenv versions --bare 2>/dev/null | awk -v prefix="${prefix}." 'index($0, prefix) == 1 { print; exit }')"
        if [ -n "$version" ]; then
            PYENV_VERSION="$version" pyenv which python 2>/dev/null && return 0
        fi
    done

    return 1
}

configure_python_for_idf() {
    local candidate=""
    local resolved=""
    local shim_dir="${REPO_ROOT}/.deps/python-bin"

    if [ -n "$IDF_BOOTSTRAP_PYTHON" ]; then
        candidate="$IDF_BOOTSTRAP_PYTHON"
    elif [ -n "${IDF_PYTHON:-}" ]; then
        candidate="$IDF_PYTHON"
    elif [ -n "${ESP_PYTHON:-}" ]; then
        candidate="$ESP_PYTHON"
    else
        candidate="$(find_pyenv_python || true)"
    fi

    if [ -n "$candidate" ]; then
        resolved="$(resolve_command "$candidate")"
    fi

    if [ -z "$resolved" ] || ! python_is_supported "$resolved"; then
        for candidate in python python3; do
            resolved="$(resolve_command "$candidate")"
            if [ -n "$resolved" ] && python_is_supported "$resolved"; then
                break
            fi
            resolved=""
        done
    fi

    if [ -z "$resolved" ]; then
        printf 'error: no supported Python found for ESP-IDF setup\n' >&2
        printf 'hint: install Python 3.10 with pyenv, or run IDF_BOOTSTRAP_PYTHON=/path/to/python make setup\n' >&2
        exit 1
    fi

    mkdir -p "$shim_dir"
    ln -sf "$resolved" "${shim_dir}/python"
    ln -sf "$resolved" "${shim_dir}/python3"
    export PATH="${shim_dir}:$PATH"

    printf 'Using Python for ESP-IDF setup: %s (%s)\n' "$resolved" "$(python_version "$resolved")"
}

repair_idf_python_env() {
    local idf_exports=""
    local idf_python_env_path=""
    local idf_env_python=""

    idf_exports="$(python "${ADF_PATH}/esp-idf/tools/idf_tools.py" export 2>/dev/null || true)"
    idf_python_env_path="$(printf '%s\n' "$idf_exports" | sed -n 's/.*export IDF_PYTHON_ENV_PATH="\([^"]*\)".*/\1/p')"

    if [ -z "$idf_python_env_path" ]; then
        printf 'warning: could not locate ESP-IDF Python environment for compatibility check\n' >&2
        return 0
    fi

    idf_env_python="${idf_python_env_path}/bin/python"
    if [ ! -x "$idf_env_python" ]; then
        printf 'warning: ESP-IDF Python interpreter not found at %s\n' "$idf_env_python" >&2
        return 0
    fi

    if ! "$idf_env_python" -c 'import pkg_resources' >/dev/null 2>&1; then
        printf 'Installing pkg_resources-compatible setuptools in %s\n' "$idf_python_env_path"
        "$idf_env_python" -m pip install --no-warn-script-location 'setuptools<70'
    fi

    if ! "$idf_env_python" "${ADF_PATH}/esp-idf/tools/check_python_dependencies.py" >/dev/null 2>&1; then
        printf 'Repairing legacy ESP-IDF Python package versions in %s\n' "$idf_python_env_path"
        "$idf_env_python" -m pip install --no-warn-script-location 'setuptools<70' 'Flask-Compress<1.15'
    fi
}

ADF_PATH="$(abs_path "$ADF_PATH")"
export ADF_REPO ADF_REF ADF_PATH IDF_TARGET

require_command git
require_command bash

mkdir -p "$(dirname "$ADF_PATH")"
configure_python_for_idf

checkout_adf_dependency

IDF_INSTALL="${ADF_PATH}/esp-idf/install.sh"
if [ ! -f "$IDF_INSTALL" ]; then
    printf 'error: ESP-IDF install script not found at %s\n' "$IDF_INSTALL" >&2
    exit 1
fi

printf 'Installing ESP-IDF tools for target %s\n' "$IDF_TARGET"
bash "$IDF_INSTALL" "$IDF_TARGET"
repair_idf_python_env

printf '\nFirmware environment ready.\n'
printf 'ADF_PATH=%s\n' "$ADF_PATH"
printf 'IDF_PATH=%s\n' "${ADF_PATH}/esp-idf"
