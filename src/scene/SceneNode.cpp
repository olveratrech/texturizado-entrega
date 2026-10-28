#include "scene/SceneNode.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

// ---------------------------------------------------------------------------
// Lote de instancias
// ---------------------------------------------------------------------------
void Lote::agregar(const glm::mat4& m, const glm::vec4& tinte) {
    transformaciones.push_back(m);
    tintes.push_back(tinte);
}

void Lote::subir() {
    if(Modelo::soloCPU)return;
    liberar();
    if (!modelo || transformaciones.empty()) return;

    // búfer intercalado: mat4 (64 bytes) + vec4 de tinte (16 bytes) por instancia
    struct Instancia {
        glm::mat4 m;
        glm::vec4 tinte;
    };
    std::vector<Instancia> datos(transformaciones.size());
    min = glm::vec3(1e30f);
    max = glm::vec3(-1e30f);
    for (size_t i = 0; i < datos.size(); ++i) {
        datos[i] = {transformaciones[i], tintes[i]};
        for (int k = 0; k < 8; ++k) {
            glm::vec3 c((k & 1) ? modelo->max.x : modelo->min.x, (k & 2) ? modelo->max.y : modelo->min.y,
                        (k & 4) ? modelo->max.z : modelo->min.z);
            glm::vec3 w = glm::vec3(transformaciones[i] * glm::vec4(c, 1.0f));
            min = glm::min(min, w);
            max = glm::max(max, w);
        }
    }
    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(datos.size() * sizeof(Instancia)), datos.data(),
                 GL_STATIC_DRAW);

    // Un VAO por primitiva que combina el búfer de vértices del modelo con el de
    // instancias (atributos 3-6: matriz por columnas, 7: tinte; divisor = 1).
    vaos_.resize(modelo->plano.size());
    for (size_t e = 0; e < modelo->plano.size(); ++e) {
        const Malla& malla = modelo->mallas[static_cast<size_t>(modelo->plano[e].malla)];
        for (const Primitiva& p : malla.primitivas) {
            GLuint vao = 0;
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, p.vbo);
            configurarAtributosVertice();
            glBindBuffer(GL_ARRAY_BUFFER, vbo_);
            for (int c = 0; c < 4; ++c) {
                glEnableVertexAttribArray(static_cast<GLuint>(3 + c));
                glVertexAttribPointer(static_cast<GLuint>(3 + c), 4, GL_FLOAT, GL_FALSE, sizeof(Instancia),
                                      reinterpret_cast<void*>(sizeof(glm::vec4) * static_cast<size_t>(c)));
                glVertexAttribDivisor(static_cast<GLuint>(3 + c), 1);
            }
            glEnableVertexAttribArray(7);
            glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(Instancia),
                                  reinterpret_cast<void*>(offsetof(Instancia, tinte)));
            glVertexAttribDivisor(7, 1);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, p.ebo);
            glBindVertexArray(0);
            vaos_[e].push_back(vao);
        }
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Lote::liberar() {
    for (auto& lista : vaos_)
        for (GLuint v : lista) glDeleteVertexArrays(1, &v);
    vaos_.clear();
    if (vbo_) glDeleteBuffers(1, &vbo_);
    vbo_ = 0;
}

// ---------------------------------------------------------------------------
// Nodo
// ---------------------------------------------------------------------------
Nodo* Nodo::agregar(std::unique_ptr<Nodo> hijo) {
    hijo->padre = this;
    hijos.push_back(std::move(hijo));
    return hijos.back().get();
}

Nodo* Nodo::agregar(const std::string& nombreHijo) {
    auto n = std::make_unique<Nodo>(nombreHijo);
    n->categoria = "grupo";
    return agregar(std::move(n));
}

Nodo* Nodo::buscar(const std::string& nombreBuscado) {
    if (nombre == nombreBuscado) return this;
    for (auto& h : hijos)
        if (Nodo* r = h->buscar(nombreBuscado)) return r;
    return nullptr;
}

void Nodo::colocar(const glm::vec3& p, const glm::vec3& rotGrados, const glm::vec3& esc) {
    posicion = p;
    rotacion = rotGrados;
    escala = esc;
}

glm::mat4 Nodo::matrizLocal() const {
    // M = T * R * S  (primero escala, luego rotación y al final traslación)
    glm::mat4 T = glm::translate(glm::mat4(1.0f), posicion);
    glm::mat4 R = glm::mat4_cast(glm::quat(glm::radians(rotacion)));
    glm::mat4 S = glm::scale(glm::mat4(1.0f), escala);
    return T * R * S;
}

void Nodo::actualizar(const glm::mat4& mundoPadre) {
    local = matrizLocal();
    mundo = mundoPadre * local;
    for (auto& h : hijos) h->actualizar(mundo);
}

int Nodo::contarNodos() const {
    int n = 1;
    for (const auto& h : hijos) n += h->contarNodos();
    return n;
}
