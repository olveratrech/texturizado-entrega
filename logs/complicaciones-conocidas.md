# Complicaciones conocidas de compilación y ejecución

## macOS

- El lanzador `.app` se probó correctamente el 28 de octubre de 2026 en Apple M1 Pro.
- La aplicación no está firmada ni notarizada. Gatekeeper puede exigir clic derecho, **Abrir**, la primera vez.
- Requiere CMake y las herramientas de línea de comandos de Xcode.
- La capa OpenGL de Apple informa una advertencia interna sobre una unidad de textura no cargable. La comprobación terminó con `GL_NO_ERROR`; la advertencia permanece en el log para investigación posterior.
- La validación especial `--verificar-apoyos`, que carga modelos sin contexto OpenGL, puede terminar con código 139. La ruta normal y `--comprobar` funcionan correctamente.

## Windows

- El lanzador fue preparado para Visual Studio 2022 x64, pero no puede verificarse desde macOS.
- Requiere la carga de trabajo **Desarrollo para el escritorio con C++** y CMake disponible en `PATH`.
- El ejecutable se enlaza como aplicación de ventana, sin consola, y con runtime MSVC estático.
- PowerShell se abre oculto mediante VBScript. Las políticas corporativas pueden bloquear scripts `.vbs` o PowerShell incluso con `ExecutionPolicy Bypass`.

## Linux

- El lanzador no puede verificarse desde macOS.
- Requiere CMake, compilador C++ y paquetes de desarrollo de OpenGL/X11 usados por GLFW.
- Algunos escritorios exigen marcar el archivo `.desktop` como confiable o seleccionar **Permitir iniciar**.
- Wayland se encuentra desactivado en la configuración de GLFW del proyecto; se espera una sesión X11 o compatibilidad XWayland.

## Registros automáticos

Cada lanzador agrega su salida a `logs/lanzador-<plataforma>.log`. La aplicación también escribe `registro.txt` junto al ejecutable compilado.
