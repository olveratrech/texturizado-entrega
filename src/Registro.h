// Registro.h - Mensajes de diagnóstico en consola y en el archivo registro.txt.
#pragma once

#include <string>

namespace registro {

// Abre el archivo de registro (si no se puede escribir, solo se usa la consola).
void abrir(const std::string& ruta);
void cerrar();

void info(const char* formato, ...);
void error(const char* formato, ...);

// Muestra un cuadro de diálogo con el error (Windows) además de registrarlo.
void fatal(const char* formato, ...);

}  // namespace registro
