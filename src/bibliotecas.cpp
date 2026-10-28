// Implementación de los decodificadores de imágenes multiplataforma.
// Limitar los formatos a los utilizados por los assets del proyecto.
#ifndef __APPLE__
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#include <stb_image.h>
#endif

// Implementacion del lector glTF.
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>
