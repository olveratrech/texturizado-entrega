// Shader.h - Programa GLSL (vértices + fragmentos) con caché de uniformes.
#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <unordered_map>

class Shader {
public:
    // "definiciones" se inserta después de #version (p. ej. "#define INSTANCIADO\n"),
    // así un mismo archivo genera variantes sin duplicar código.
    bool cargar(const std::string& rutaVert, const std::string& rutaFrag, const std::string& definiciones = "");
    void usar() const { glUseProgram(id_); }
    void liberar();

    GLint ubicacion(const char* nombre) const;

    void set(const char* n, int v) const { glUniform1i(ubicacion(n), v); }
    void set(const char* n, float v) const { glUniform1f(ubicacion(n), v); }
    void set(const char* n, const glm::vec2& v) const { glUniform2fv(ubicacion(n), 1, &v.x); }
    void set(const char* n, const glm::vec3& v) const { glUniform3fv(ubicacion(n), 1, &v.x); }
    void set(const char* n, const glm::vec4& v) const { glUniform4fv(ubicacion(n), 1, &v.x); }
    void set(const char* n, const glm::mat3& m) const { glUniformMatrix3fv(ubicacion(n), 1, GL_FALSE, &m[0][0]); }
    void set(const char* n, const glm::mat4& m) const { glUniformMatrix4fv(ubicacion(n), 1, GL_FALSE, &m[0][0]); }

    GLuint id() const { return id_; }

private:
    GLuint id_ = 0;
    mutable std::unordered_map<std::string, GLint> cache_;
};
