#include "Registro.h"

#include <cstdarg>
#include <cstdio>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace registro {

static FILE* g_archivo = nullptr;

void abrir(const std::string& ruta) {
    g_archivo = std::fopen(ruta.c_str(), "w");
}

void cerrar() {
    if (g_archivo) std::fclose(g_archivo);
    g_archivo = nullptr;
}

static void escribir(const char* prefijo, const char* formato, va_list args) {
    char buffer[2048];
    std::vsnprintf(buffer, sizeof(buffer), formato, args);
    std::fprintf(stderr, "%s%s\n", prefijo, buffer);
    if (g_archivo) {
        std::fprintf(g_archivo, "%s%s\n", prefijo, buffer);
        std::fflush(g_archivo);
    }
}

void info(const char* formato, ...) {
    va_list args;
    va_start(args, formato);
    escribir("[info] ", formato, args);
    va_end(args);
}

void error(const char* formato, ...) {
    va_list args;
    va_start(args, formato);
    escribir("[error] ", formato, args);
    va_end(args);
}

void fatal(const char* formato, ...) {
    char buffer[2048];
    va_list args;
    va_start(args, formato);
    std::vsnprintf(buffer, sizeof(buffer), formato, args);
    va_end(args);
    error("%s", buffer);
#ifdef _WIN32
    MessageBoxA(nullptr, buffer, "Cempasúchil", MB_OK | MB_ICONERROR);
#endif
}

}  // namespace registro
