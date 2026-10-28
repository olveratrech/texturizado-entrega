// Rutas.h - Localiza la carpeta base del programa (la que contiene assets/ y shaders/).
//
// Se buscan, en orden: la carpeta del ejecutable, el directorio de trabajo y
// hasta dos niveles hacia arriba de ambos. Así funciona igual al ejecutar
// desde Visual Studio, desde la carpeta build/ o desde la instalación final.
#pragma once

#include <string>

namespace rutas {

bool inicializar(const char* argv0);
const std::string& base();
std::string asset(const std::string& relativa);
std::string shader(const std::string& nombre);

}  // namespace rutas
