#!/bin/sh
set -u

DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$DIR/../.." && pwd)
LOG_DIR="$ROOT/logs"
LOG="$LOG_DIR/lanzador-linux.log"
BUILD="$ROOT/build-linux"
mkdir -p "$LOG_DIR"
printf '\n===== %s Linux =====\n' "$(date '+%Y-%m-%d %H:%M:%S')" >>"$LOG"

if ! command -v cmake >/dev/null 2>&1; then
  echo "ERROR: No se encontró CMake." >>"$LOG"
  command -v notify-send >/dev/null 2>&1 && notify-send "Texturizado Cempasúchil" "No se encontró CMake. Revisa la carpeta logs."
  exit 3
fi
if ! cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release >>"$LOG" 2>&1 ||
   ! cmake --build "$BUILD" --parallel 4 >>"$LOG" 2>&1; then
  command -v notify-send >/dev/null 2>&1 && notify-send "Texturizado Cempasúchil" "Falló la compilación. Revisa la carpeta logs."
  exit 4
fi
cmake -E copy_directory "$ROOT/assets" "$BUILD/assets" >>"$LOG" 2>&1
cmake -E copy_directory "$ROOT/shaders" "$BUILD/shaders" >>"$LOG" 2>&1
cd "$BUILD" || exit 5
nohup "$BUILD/TexturizadoCempasuchil" >>"$LOG" 2>&1 &
