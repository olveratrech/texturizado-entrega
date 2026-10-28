#pragma once

#include "render/Shader.h"
#include <glm/glm.hpp>
#include <string>

class Skybox {
public:
    bool iniciar(const std::string& carpetaCaras);
    void dibujar(const glm::mat4& vista, const glm::mat4& proyeccion) const;
    void liberar();
    bool listo() const { return cubo_ != 0; }

private:
    Shader shader_;
    GLuint vao_ = 0, vbo_ = 0, cubo_ = 0;
};
