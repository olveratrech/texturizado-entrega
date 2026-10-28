# Lanzadores de la escena

Todos los lanzadores configuran y compilan una versión `Release`, actualizan `assets/` y
`shaders/`, y finalmente abren `TexturizadoCempasuchil` sin procesos en terminal. La salida
técnica se guarda en `logs/` en la raíz del proyecto.

## macOS

Abrir `macOS/TexturizadoCempasuchil-0.2.1.app`. Requiere CMake y las herramientas de línea de
comandos de Xcode. macOS puede pedir confirmar **Abrir** la primera vez por tratarse de una
aplicación local sin firma. En caso de bloqueo: clic derecho sobre la aplicación, **Abrir**.

## Windows

Abrir `windows/TexturizadoCempasuchil.vbs`. Requiere CMake y Visual Studio 2022 con la carga
de trabajo **Desarrollo para el escritorio con C++**. PowerShell y la compilación permanecen
ocultos; los errores se guardan en `logs/lanzador-windows.log`.

## Linux

Marcar `linux/TexturizadoCempasuchil.desktop` como confiable/ejecutable y abrirlo. También
puede ejecutarse `linux/iniciar_linux.sh`. Requiere CMake, un compilador C++ y las bibliotecas
de desarrollo X11/OpenGL solicitadas por GLFW. Algunas distribuciones bloquean archivos
`.desktop` descargados hasta elegir **Permitir iniciar**.

## Diagnóstico documentable

- `logs/lanzador-macos.log`
- `logs/lanzador-windows.log`
- `logs/lanzador-linux.log`
- `build-*/registro.txt` para mensajes internos de la aplicación.

Los lanzadores no descargan dependencias: GLFW, GLAD, GLM y cgltf ya forman parte del
proyecto. Si falta CMake, compilador, SDK gráfico o permisos del sistema, el detalle queda
registrado en el log correspondiente.
