#!/usr/bin/env bash
set -euo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
project_root=$(cd "$script_dir/.." && pwd)
image='devkitpro/devkita64@sha256:1fc388c3a0d34bd2045a6dadcb1020e069d5f876a187fd705de14b4440c00282'
sdl_stage=${SDL3_STAGE:-"$project_root/build/switch/vendor/SDL/build-switch/stage"}

if [[ -z "$sdl_stage" ]]; then
    echo "Informe o caminho do SDL3 para Switch em SDL3_STAGE." >&2
    echo "Exemplo: SDL3_STAGE=/c/caminho/para/stage bash tools/build-switch.sh" >&2
    exit 1
fi
if [[ ! -d "$sdl_stage/include/SDL3" || ! -f "$sdl_stage/lib/libSDL3.a" ]]; then
    echo "SDL3_STAGE deve conter include/SDL3 e lib/libSDL3.a: $sdl_stage" >&2
    exit 1
fi
if [[ ! -s "$project_root/assets/icon.jpg" ]]; then
    echo "Ícone do jogo ausente em $project_root/assets/icon.jpg." >&2
    exit 1
fi

sdl_stage=$(cd "$sdl_stage" && pwd)
if command -v cygpath >/dev/null 2>&1; then
    docker_project_root=$(cygpath -m "$project_root")
    docker_sdl_stage=$(cygpath -m "$sdl_stage")
else
    docker_project_root=$project_root
    docker_sdl_stage=$sdl_stage
fi

docker_user="$(id -u):$(id -g)"
asset_output="$project_root/build/switch/romfs/assets"
if [[ -d "$asset_output" ]] && command -v stat >/dev/null 2>&1; then
    docker_user=$(stat -c '%u:%g' "$asset_output" 2>/dev/null || printf '%s' "$docker_user")
fi

MSYS_NO_PATHCONV=1 docker run --rm --user "$docker_user" --mount "type=bind,source=$docker_project_root,target=/work/tamandua-bandeira" --mount "type=bind,source=$docker_sdl_stage,target=/opt/SDL3,readonly" --workdir=/work/tamandua-bandeira "$image" sh -ec '
    rm -rf build/switch/game/romfs/assets
    python3 tools/prepare_assets.py assets/source build/switch/game/romfs/assets
    python3 tools/compose_music.py build/switch/game/romfs/assets/rock-guitarra.wav
    python3 tools/compose_sfx.py build/switch/game/romfs/assets/impacto-golpe.wav
    rm -f build/switch/game/romfs/assets/rock-teclado.wav
    build_dir=/tmp/tamandua-switch-build
    mkdir -p "$build_dir/romfs/assets"
    cp build/switch/game/romfs/assets/* "$build_dir/romfs/assets/"
    cmake -S . -B "$build_dir" -G Ninja -DCMAKE_TOOLCHAIN_FILE=/opt/devkitpro/cmake/Switch.cmake -DCMAKE_PREFIX_PATH=/opt/SDL3
    cmake --build "$build_dir" -j4
    cp "$build_dir/tamandua_bandeira.nro" build/switch/TamanduaBandeira.nro
    echo "NRO pronto: /work/tamandua-bandeira/build/switch/TamanduaBandeira.nro"
    '
