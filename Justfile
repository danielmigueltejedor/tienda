# These are just convenience scripts, NOT a build system!

appid := env("STORE_APPID", "os.nodalix.Store")
manifest := "./build-aux/flatpak/" + appid + ".json"
branch := env("STORE_BRANCH", "master")
just := just_executable()

alias run := run-base

run-base: build-base
    ./build/src/nodalix-store

build-base:
    meson setup build --wipe
    ninja -C build

build-flatpak $manifest=manifest $branch=branch:
    #!/usr/bin/env bash
    set -xeuo pipefail
    flatpak --user remote-add --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo

    FLATPAK_BUILDER_DIR="$(realpath ".flatpak-builder")"
    BUILDER_ARGS+=(--default-branch="${branch}")
    BUILDER_ARGS+=(--state-dir="${FLATPAK_BUILDER_DIR}/flatpak-builder")
    BUILDER_ARGS+=("--user")
    BUILDER_ARGS+=("--ccache")
    BUILDER_ARGS+=("--force-clean")
    BUILDER_ARGS+=("--install")
    BUILDER_ARGS+=("--disable-updates")
    BUILDER_ARGS+=("--disable-rofiles-fuse")
    BUILDER_ARGS+=("${FLATPAK_BUILDER_DIR}/build-dir")
    BUILDER_ARGS+=("${manifest}")

    if which flatpak-builder &>/dev/null ; then
        flatpak-builder "${BUILDER_ARGS[@]}"
    else
        flatpak run org.flatpak.Builder "${BUILDER_ARGS[@]}"
    fi

build-flatpak-bundle $appid=appid: build-flatpak
    #!/usr/bin/env bash
    set -euo pipefail

    FLATPAK_BUILDER_DIR="$(realpath ".flatpak-builder")"

    flatpak build-export repo "${FLATPAK_BUILDER_DIR}/build-dir"
    flatpak build-bundle repo "${appid}".flatpak "${appid}"
