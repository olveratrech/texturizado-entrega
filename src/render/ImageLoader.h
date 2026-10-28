#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

struct ImagenRGBA {
    int ancho = 0;
    int alto = 0;
    std::vector<std::uint8_t> pixeles;
};

// Decodifica PNG/JPEG desde memoria. En macOS usa ImageIO, que ya forma parte
// del sistema y evita incorporar otra copia de una biblioteca de imágenes.
bool decodificarImagen(const void* datos, std::size_t tam, ImagenRGBA& salida);
