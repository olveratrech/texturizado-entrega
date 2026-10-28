#pragma once

#include "render/Shader.h"
#include "render/Skybox.h"
#include "scene/Camera.h"
#include "scene/SceneNode.h"

#include <glm/glm.hpp>

#include <vector>

struct Estadisticas {
    int llamadas = 0;     // draw calls
    int triangulos = 0;
    int instancias = 0;
    int descartados = 0;  // primitivas fuera del frustum
    int luces = 0;
};

class Renderer {
public:
    bool iniciar();
    void liberar();

    void dibujar(Nodo* raiz, const Camara& camara, int ancho, int alto, const Skybox* cielo = nullptr);

    bool alambre = false;
    bool descartar = true;  // descarte por frustum activado
    bool spotlight = false; // linterna ligada a la cámara (tecla F)
    bool lucesPuntuales = true; // interruptor para diagnóstico comparativo
    Estadisticas stats;

private:
    enum class ZonaLuz { Otra, Altar, Camino, Cementerio };
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
    struct LuzPuntual {
        glm::vec3 posicion{0.0f};
        glm::vec3 color{1.0f};
        float distancia2 = 0.0f;
        float lineal = 0.7f;
        float cuadratica = 1.8f;
        float radio2 = 9.0f;
        int cantidad = 1;
        bool farol = false;
        bool relleno = false;
        float factorVisibilidad = 1.0f;
        ZonaLuz zona = ZonaLuz::Otra;
    };

    void recolectar(Nodo* n, const glm::vec4& tintePadre = glm::vec4(1.0f),
                    ZonaLuz zona = ZonaLuz::Otra);
    bool visible(const glm::vec3& centro, float radio) const;
    void emitir(const Dibujo& d);
    void aplicarMaterial(const Shader& s, const Material& m);
    void aplicarLuces(const Shader& s, const Camara& camara) const;
    void registrarLuz(const Material& material, const glm::vec3& posicion, ZonaLuz zona,
                      float factorVisibilidad = 1.0f);
    void agruparLucesAltar();
    float visibilidadPantalla(const glm::vec3& posicion) const;

    Shader shader_, shaderInst_;
    std::vector<Dibujo> opacos_;
    std::vector<Dibujo> transparentes_;
    std::vector<LuzPuntual> luces_;
    Plano frustum_[6];
    glm::mat4 vistaProy_{1.0f};
    glm::vec3 ojo_{0.0f};
};
