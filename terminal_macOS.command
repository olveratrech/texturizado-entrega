#!/bin/zsh

set -eu

cd "$(dirname "$0")"

export DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer

cmake_texturizado="$(command -v cmake || true)"

if [[ -z "$cmake_texturizado" ]]; then
  for candidato in /opt/homebrew/bin/cmake /Applications/CMake.app/Contents/bin/cmake /Applications/MATLAB_R2024b.app/bin/maca64/cmake/bin/cmake; do
    if [[ -x "$candidato" ]]; then cmake_texturizado="$candidato"; break; fi
  done
fi

if [[ -z "$cmake_texturizado" ]]; then
  echo 'No se encontro CMake. Instala CMake y vuelve a abrir este archivo.'
  read -r respuesta
  exit 1
fi

"$cmake_texturizado" -S . -B build-xcode -G Xcode

open build-xcode/TexturizadoCempasuchil.xcodeproj
