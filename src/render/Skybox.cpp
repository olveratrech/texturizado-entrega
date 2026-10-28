#include "render/Skybox.h"

#include "Registro.h"
#include "Rutas.h"
#include "render/ImageLoader.h"

#include <filesystem>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;

static bool leerImagen(const fs::path& ruta, ImagenRGBA& imagen) {
    std::ifstream archivo(ruta, std::ios::binary | std::ios::ate);
    if (!archivo) return false;
    const std::streamsize tam = archivo.tellg();
    if (tam <= 0) return false;
    std::vector<std::uint8_t> datos(static_cast<std::size_t>(tam));
    archivo.seekg(0);
    return archivo.read(reinterpret_cast<char*>(datos.data()), tam) &&
           decodificarImagen(datos.data(), datos.size(), imagen);
}

bool Skybox::iniciar(const std::string& carpetaCaras) {
    if (!shader_.cargar(rutas::shader("cielo.vert"), rutas::shader("cielo.frag"))) return false;
    static const char* caras[] = {"px", "nx", "py", "ny", "pz", "nz"};
    glGenTextures(1, &cubo_);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubo_);
    for (int i = 0; i < 6; ++i) {
        fs::path ruta;
        for (const char* extension : {".jpg", ".png"}) {
            const fs::path candidata = fs::u8path(carpetaCaras) / (std::string(caras[i]) + extension);
            if (fs::exists(candidata)) { ruta = candidata; break; }
        }
        ImagenRGBA imagen;
        if (ruta.empty() || !leerImagen(ruta, imagen)) {
            registro::error("No se pudo cargar la cara %s del skybox", caras[i]);
            liberar();
            return false;
        }
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_SRGB8_ALPHA8,
                     imagen.ancho, imagen.alto, 0, GL_RGBA, GL_UNSIGNED_BYTE, imagen.pixeles.data());
    }
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    static const float vertices[] = {
        -1,1,-1,-1,-1,-1, 1,-1,-1, 1,-1,-1, 1,1,-1,-1,1,-1,
        -1,-1,1,-1,-1,-1,-1,1,-1,-1,1,-1,-1,1,1,-1,-1,1,
         1,-1,-1,1,-1,1,1,1,1,1,1,1,1,1,-1,1,-1,-1,
        -1,-1,1,-1,1,1,1,1,1,1,1,1,1,-1,1,-1,-1,1,
        -1,1,-1,1,1,-1,1,1,1,1,1,1,-1,1,1,-1,1,-1,
        -1,-1,-1,-1,-1,1,1,-1,-1,1,-1,-1,-1,-1,1,1,-1,1
    };
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glBindVertexArray(0);
    return true;
}

void Skybox::dibujar(const glm::mat4& vista, const glm::mat4& proyeccion) const {
    if (!listo()) return;
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    shader_.usar();
    shader_.set("uVistaSinTraslacion", glm::mat4(glm::mat3(vista)));
    shader_.set("uProyeccion", proyeccion);
    shader_.set("uCielo", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubo_);
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
}

void Skybox::liberar() {
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (cubo_) glDeleteTextures(1, &cubo_);
    vao_ = vbo_ = cubo_ = 0;
    shader_.liberar();
}
