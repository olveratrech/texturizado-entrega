#include "render/ImageLoader.h"

#include <algorithm>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>
#endif

bool decodificarImagen(const void* datos, std::size_t tam, ImagenRGBA& salida) {
#ifdef __APPLE__
    if (!datos || tam == 0) return false;
    CFDataRef bloque = CFDataCreate(kCFAllocatorDefault, static_cast<const UInt8*>(datos),
                                    static_cast<CFIndex>(tam));
    if (!bloque) return false;
    CGImageSourceRef fuente = CGImageSourceCreateWithData(bloque, nullptr);
    CFRelease(bloque);
    if (!fuente) return false;
    CGImageRef imagen = CGImageSourceCreateImageAtIndex(fuente, 0, nullptr);
    CFRelease(fuente);
    if (!imagen) return false;

    salida.ancho = static_cast<int>(CGImageGetWidth(imagen));
    salida.alto = static_cast<int>(CGImageGetHeight(imagen));
    salida.pixeles.assign(static_cast<std::size_t>(salida.ancho) * salida.alto * 4, 0);
    CGColorSpaceRef espacio = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    CGContextRef contexto = CGBitmapContextCreate(
        salida.pixeles.data(), salida.ancho, salida.alto, 8,
        static_cast<std::size_t>(salida.ancho) * 4, espacio,
        kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
    CGColorSpaceRelease(espacio);
    if (!contexto) {
        CGImageRelease(imagen);
        salida = {};
        return false;
    }
    CGContextSetBlendMode(contexto, kCGBlendModeCopy);
    CGContextDrawImage(contexto, CGRectMake(0, 0, salida.ancho, salida.alto), imagen);
    CGContextRelease(contexto);
    CGImageRelease(imagen);
    // CoreGraphics entrega RGBA premultiplicado; glTF almacena color y alfa
    // independientes. Despremultiplicar evita bordes oscuros en recortes.
    for (std::size_t i = 0; i < salida.pixeles.size(); i += 4) {
        const unsigned a = salida.pixeles[i + 3];
        if (a > 0 && a < 255)
            for (int c = 0; c < 3; ++c)
                salida.pixeles[i + c] = static_cast<std::uint8_t>(
                    std::min(255u, (unsigned(salida.pixeles[i + c]) * 255u + a / 2u) / a));
    }
    return true;
#else
    (void)datos;
    (void)tam;
    (void)salida;
    return false;
#endif
}
