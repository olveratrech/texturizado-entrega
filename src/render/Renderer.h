#pragma once

#include "render/Shader.h"
#include "scene/Camera.h"
#include "scene/SceneNode.h"

#include <glm/glm.hpp>

#include <vector>

struct Estadisticas {
    int llamadas = 0;     // draw calls
    int triangulos = 0;
    int instancias = 0;
    int descartados = 0;  // primitivas fuera del frustum
};

class Renderer {
public:
    bool iniciar();
    void liberar();

    void dibujar(Nodo* raiz, const Camara& camara, int ancho, int alto);

    bool alambre = false;
    bool descartar = true;  // descarte por frustum activado
    Estadisticas stats;

private:
    struct Dibujo {
        const Primitiva* prim = nullptr;
        const Material* mat = nullptr;
        glm::mat4 matriz{1.0f};   // mundo (o matriz del nodo dentro del modelo si es instanciado)
        glm::mat4 padre{1.0f};    // mundo del dueño del lote
        const Lote* lote = nullptr;
        GLuint vaoInstancias = 0;
        glm::vec4 tinte{1.0f};
        float distancia2 = 0.0f;
    };
    struct Plano {
        glm::vec3 n;
        float d;
    };

    void recolectar(Nodo* n, const glm::vec4& tintePadre = glm::vec4(1.0f));
    bool visible(const glm::vec3& centro, float radio) const;
    void emitir(const Dibujo& d);
    void aplicarMaterial(const Shader& s, const Material& m);

    Shader shader_, shaderInst_;
    std::vector<Dibujo> opacos_;
    std::vector<Dibujo> transparentes_;
    Plano frustum_[6];
    glm::mat4 vistaProy_{1.0f};
    glm::vec3 ojo_{0.0f};
};
