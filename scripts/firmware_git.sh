#!/usr/bin/env bash

: "${DEPS_CLONE_DEPTH:=1}"
if ! [[ "$DEPS_CLONE_DEPTH" =~ ^(0|[1-9][0-9]*)$ ]]; then
    printf 'error: DEPS_CLONE_DEPTH must be a non-negative integer (0 means full history)\n' >&2
    return 1
fi
export DEPS_CLONE_DEPTH

update_dependency_submodules() {
    local repo_path="$1"
    local update_args=(submodule update --init --recursive)
    if [ "$DEPS_CLONE_DEPTH" != "0" ]; then
        update_args+=(--depth "$DEPS_CLONE_DEPTH")
    else
        update_args+=(--no-recommend-shallow)
    fi

    git -C "$repo_path" submodule sync --recursive
    git -C "$repo_path" "${update_args[@]}"
}

checkout_adf_dependency() {
    local clone_args=(clone --no-tags --branch "$ADF_REF")
    local fetch_args=(fetch --no-tags --recurse-submodules=no)
    local current_url
    if [ "$DEPS_CLONE_DEPTH" != "0" ]; then
        clone_args+=(--depth "$DEPS_CLONE_DEPTH" --single-branch)
        fetch_args+=(--depth "$DEPS_CLONE_DEPTH")
    fi

    if [ ! -e "${ADF_PATH}/.git" ]; then
        if [ -d "$ADF_PATH" ] && [ -n "$(find "$ADF_PATH" -mindepth 1 -maxdepth 1 -print -quit)" ]; then
            printf 'error: %s exists but is not an ESP-ADF git checkout\n' "$ADF_PATH" >&2
            return 1
        fi
        printf 'Cloning ESP-ADF %s into %s (history depth: %s)\n' "$ADF_REF" "$ADF_PATH" "$DEPS_CLONE_DEPTH"
        git "${clone_args[@]}" "$ADF_REPO" "$ADF_PATH"
    else
        printf 'Using ESP-ADF checkout at %s\n' "$ADF_PATH"
        current_url="$(git -C "$ADF_PATH" config --get remote.origin.url || true)"
        if [ -n "$current_url" ] && [ "$current_url" != "$ADF_REPO" ]; then
            printf 'warning: ESP-ADF remote is %s, expected %s\n' "$current_url" "$ADF_REPO" >&2
        fi

        if [ "${ADF_SKIP_UPDATE:-0}" != "1" ]; then
            if git -C "$ADF_PATH" "${fetch_args[@]}" origin "$ADF_REF"; then
                # FETCH_HEAD works for both branches and tags without fetching other tags.
                git -C "$ADF_PATH" checkout --detach FETCH_HEAD
            else
                printf 'warning: could not update ESP-ADF; continuing with existing checkout\n' >&2
            fi
        fi
    fi

    update_dependency_submodules "$ADF_PATH"
}
