#!/usr/bin/env bash
set -euo pipefail

script_dir=$(cd "$(dirname "$0")" && pwd)
project_root=$(cd "$script_dir/.." && pwd)
image='emscripten/emsdk:7.0.1'

if [[ ! -s "$project_root/assets/icon.jpg" ]]; then
    echo "Ícone do jogo ausente em assets/icon.jpg." >&2
    exit 1
fi

if command -v cygpath >/dev/null 2>&1; then
    docker_project_root=$(cygpath -m "$project_root")
else
    docker_project_root=$project_root
fi

mkdir -p "$project_root/build/web/romfs/assets" "$project_root/build/web/site"
docker_user="$(id -u):$(id -g)"
if command -v stat >/dev/null 2>&1; then
    docker_user=$(stat -c '%u:%g' "$project_root/build/web/site" 2>/dev/null || printf '%s' "$docker_user")
fi

MSYS_NO_PATHCONV=1 docker run --rm --user "$docker_user" \
    --mount "type=bind,source=$docker_project_root,target=/work/tamandua-bandeira" \
    --workdir=/work/tamandua-bandeira "$image" sh -ec '
    rm -rf build/web/romfs/assets/*
    python3 tools/prepare_assets.py assets/source build/web/romfs/assets
    python3 tools/compose_music.py build/web/romfs/assets/rock-guitarra.wav
    python3 tools/compose_sfx.py build/web/romfs/assets/impacto-golpe.wav
    rm -f build/web/romfs/assets/rock-teclado.wav
    em++ -std=c++20 -O3 -sUSE_SDL=3 -sASYNCIFY=1 \
        -sALLOW_MEMORY_GROWTH=1 -sMAXIMUM_MEMORY=1gb \
        -sEXPORTED_RUNTIME_METHODS=callMain \
        --preload-file build/web/romfs/assets@/assets \
        --shell-file web/shell.html \
        src/main.cpp -o build/web/site/index.html
    echo "Versão web pronta em build/web/site/index.html"
'
