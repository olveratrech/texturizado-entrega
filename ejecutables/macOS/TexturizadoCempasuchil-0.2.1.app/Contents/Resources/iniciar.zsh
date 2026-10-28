#!/bin/zsh
set -u

directorio="$(cd "$(dirname "$0")" && pwd)"
raiz="$directorio"
while [[ "$raiz" != "/" && ! -f "$raiz/CMakeLists.txt" ]]; do raiz="$(dirname "$raiz")"; done
if [[ ! -f "$raiz/CMakeLists.txt" ]]; then
  echo "ERROR: No se encontró CMakeLists.txt desde $directorio" >>/tmp/TexturizadoCempasuchil-macOS.log
  /usr/bin/osascript -e 'display dialog "No se encontró la carpeta del proyecto. El .app debe permanecer dentro de ejecutables/macOS." buttons {"Aceptar"} with icon stop'
  exit 2
fi

mkdir -p "$raiz/logs"
log="$raiz/logs/lanzador-macos.log"
printf '\n===== %s macOS app 0.2.2 =====\nProyecto: %s\n' "$(date '+%Y-%m-%d %H:%M:%S')" "$raiz" >>"$log"
cmake_bin="$(command -v cmake 2>/dev/null || true)"
if [[ -z "$cmake_bin" ]]; then
  for candidato in /opt/homebrew/bin/cmake /Applications/CMake.app/Contents/bin/cmake /Applications/MATLAB_R2024b.app/bin/maca64/cmake/bin/cmake; do
    if [[ -x "$candidato" ]]; then cmake_bin="$candidato"; break; fi
  done
fi
if [[ -z "$cmake_bin" ]]; then
  echo "ERROR: No se encontró CMake." >>"$log"
  /usr/bin/osascript -e 'display dialog "No se encontró CMake. Revisa logs/lanzador-macos.log" buttons {"Aceptar"} with icon stop'
  exit 3
fi

[[ -d /Applications/Xcode.app ]] && export DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer
build="$raiz/build-mac-app"
ejecutable="$build/TexturizadoCempasuchil"
compilacion_correcta=true
"$cmake_bin" -S "$raiz" -B "$build" -DCMAKE_BUILD_TYPE=Release >>"$log" 2>&1 || compilacion_correcta=false
if [[ "$compilacion_correcta" == true ]]; then
  "$cmake_bin" --build "$build" --config Release -j 4 >>"$log" 2>&1 || compilacion_correcta=false
fi
if [[ "$compilacion_correcta" != true ]]; then
  if [[ -x "$ejecutable" ]]; then
    echo "ADVERTENCIA: falló la recompilación; se usará el último ejecutable válido." >>"$log"
  else
    /usr/bin/osascript -e 'display dialog "No se pudo compilar Texturizado Cempasúchil. Revisa logs/lanzador-macos.log" buttons {"Aceptar"} with icon stop'
    exit 4
  fi
fi
"$cmake_bin" -E copy_directory "$raiz/assets" "$build/assets" >>"$log" 2>&1
"$cmake_bin" -E copy_directory "$raiz/shaders" "$build/shaders" >>"$log" 2>&1
cd "$build"
exec "$ejecutable" "$@" >>"$log" 2>&1
