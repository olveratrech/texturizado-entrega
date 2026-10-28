# Cempasúchil — Modelado

Visor 3D de un altar de Día de Muertos y su entorno. Esta entrega presenta los modelos en gris mate, con iluminación sencilla y vista de alambre. Para el desarrollo, las pruebas y los resultados consulta [la documentación](doc/Equipo-8-modelado.pdf).

## Requisitos

- Tarjeta gráfica y controlador compatibles con OpenGL 3.3 o superior.
- Para compilar: CMake 3.16 o posterior y un compilador de C++17. En macOS se usa Xcode; en Windows, Visual Studio 2022 con «Desarrollo para el escritorio con C++».
- Las dependencias del código están incluidas en `external/`.

## Compilar y ejecutar

Ejecuta los comandos desde la raíz de esta carpeta.

**macOS con Xcode:** abre `terminal_macOS.command`, o ejecuta:

```sh
cmake -S . -B build-xcode -G Xcode
open build-xcode/ModeladoCempasuchil.xcodeproj
```

En Xcode selecciona el esquema **ModeladoCempasuchil**, destino **My Mac**, y pulsa **⌘R**.

**Windows con Visual Studio 2022:**

```bat
cmake -S . -B build-windows -G "Visual Studio 17 2022" -A x64
cmake --build build-windows --config Release
```

Ejecuta `build-windows\Release\ModeladoCempasuchil.exe`.

**Linux:** instala CMake, un compilador C++ y los paquetes de desarrollo que requiere GLFW en tu distribución. Después:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/ModeladoCempasuchil
```

CMake copia `assets/`, `shaders/` y `readme.txt` junto al ejecutable. Conserva esos elementos al distribuirlo. Las carpetas `build`, `build-xcode` y `build-windows` son generadas localmente.

## Controles

| Acción | Teclas |
|---|---|
| Desplazarse | W, A, S, D |
| Bajar y subir | Q, E |
| Mirar alrededor | Mantener botón derecho y mover el ratón |
| Acelerar | Shift |
| Ajustar velocidad | Rueda del ratón |
| Vista sólida / alambre | F2 |
| Volver al inicio | R |
| Salir | Esc |

En algunos teclados Mac se usa **Fn + F2**.

## Archivos principales

`src/` contiene el programa; `shaders/`, los dos shaders de la escena; `assets/models/`, los modelos propios y de librería; `external/`, las bibliotecas necesarias para compilar. `CMakeLists.txt` define la compilación. `doc/` contiene el reporte. Las atribuciones y licencias de los recursos están en [CREDITOS.md](assets/CREDITOS.md).

## Altar aprobado integrado

La ofrenda fue sustituida por la composición aprobada, con los ajustes de posición, escala y ancho del petate autorizados. Detalles y comprobaciones en [INTEGRACION_ALTAR.md](doc/INTEGRACION_ALTAR.md). Para abrir directamente su vista: `./build-xcode/Release/ModeladoCempasuchil --altar`.
