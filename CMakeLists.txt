cmake_minimum_required(VERSION 3.16...3.31)

project(TexturizadoCempasuchil
    VERSION 0.2.0
    DESCRIPTION "Camino de Cempasuchil - Proyecto final CGeIHC, FI UNAM"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Tipo de compilación" FORCE)
endif()

# En Windows se enlaza el runtime de MSVC de forma estática para que el .exe
# funcione en equipos sin el paquete redistribuible de Visual C++.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# --------------------------------------------------------------------------
# Dependencias (incluidas en external/, no requieren conexión a internet)
# --------------------------------------------------------------------------
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glfw EXCLUDE_FROM_ALL)

add_library(glad STATIC external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

# --------------------------------------------------------------------------
# Programa
# --------------------------------------------------------------------------
add_executable(TexturizadoCempasuchil
    src/main.cpp
    src/App.cpp
    src/Registro.cpp
    src/Rutas.cpp
    src/render/Shader.cpp
    src/render/Model.cpp
    src/render/Renderer.cpp
    src/scene/Camera.cpp
    src/scene/SceneNode.cpp
    src/scene/Camino.cpp
    src/scene/Escena.cpp
    src/scene/AltarAprobado.cpp
    src/bibliotecas.cpp)

# Generar un esquema compartido con ejecutable de Run definido.
set_target_properties(TexturizadoCempasuchil PROPERTIES
    XCODE_GENERATE_SCHEME TRUE
    XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

target_include_directories(TexturizadoCempasuchil PRIVATE
    src
    external/glm
    external/cgltf)

target_link_libraries(TexturizadoCempasuchil PRIVATE glfw glad)
target_compile_definitions(TexturizadoCempasuchil PRIVATE GLM_ENABLE_EXPERIMENTAL)

if(MSVC)
    target_compile_options(TexturizadoCempasuchil PRIVATE /W3 /utf-8)
    target_compile_definitions(TexturizadoCempasuchil PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
    # aplicación de ventana (sin consola); los mensajes se guardan en registro.txt
    set_target_properties(TexturizadoCempasuchil PROPERTIES
        WIN32_EXECUTABLE TRUE
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
    target_link_options(TexturizadoCempasuchil PRIVATE /ENTRY:mainCRTStartup)
else()
    target_compile_options(TexturizadoCempasuchil PRIVATE -Wall -Wextra -Wno-unused-parameter)
    if(WIN32)
        target_link_options(TexturizadoCempasuchil PRIVATE -mwindows -static)
    endif()
endif()

# Las bibliotecas de un solo encabezado se compilan sin advertencias (código de terceros)
if(MSVC)
    set_source_files_properties(src/bibliotecas.cpp PROPERTIES COMPILE_OPTIONS "/W0")
else()
    set_source_files_properties(src/bibliotecas.cpp PROPERTIES COMPILE_OPTIONS "-w")
endif()

# Copia assets/ y shaders/ junto al ejecutable para que la carpeta de salida
# se pueda empaquetar directamente (InstallForge).
add_custom_command(TARGET TexturizadoCempasuchil POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:TexturizadoCempasuchil>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:TexturizadoCempasuchil>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/readme.txt $<TARGET_FILE_DIR:TexturizadoCempasuchil>/readme.txt
    COMMENT "Copiando assets, shaders y readme.txt junto al ejecutable")
