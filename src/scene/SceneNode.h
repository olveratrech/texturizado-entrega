// SceneNode.h - Grafo de escena jerárquico.
//
// Cada nodo guarda su transformación LOCAL (traslación, rotación en grados
// XYZ y escala) relativa a su padre. La matriz de MUNDO se obtiene al recorrer
// el árbol:  M_mundo(hijo) = M_mundo(padre) * T * R * S
// Así, al mover el altar se mueven todas sus ofrendas, al girar el brazo de un
// farol gira también la lámpara, etc. (modelado jerárquico).
#pragma once

#include "render/Model.h"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <vector>

// Lote de instancias: un mismo modelo repetido N veces con una sola llamada de
// dibujo por primitiva (glDrawElementsInstanced). Se usa para los miles de
// pétalos del camino, veladoras, flores de las tumbas, bardas, etc.
class Lote {
public:
    const Modelo* modelo = nullptr;
    std::vector<glm::mat4> transformaciones;  // relativas al nodo dueño del lote
    std::vector<glm::vec4> tintes;            // color multiplicativo por instancia

    void agregar(const glm::mat4& m, const glm::vec4& tinte = glm::vec4(1.0f));
    void subir();  // crea el búfer de instancias y los VAOs
    void liberar();
    ~Lote() { liberar(); }

    int instancias() const { return static_cast<int>(transformaciones.size()); }
    const std::vector<GLuint>& vaosDe(size_t plano) const { return vaos_[plano]; }
    glm::vec3 min{0.0f}, max{0.0f};  // caja envolvente de todas las instancias

private:
    GLuint vbo_ = 0;
    std::vector<std::vector<GLuint>> vaos_;  // [entrada del plano][primitiva]
};

class Nodo {
public:
    explicit Nodo(std::string n) : nombre(std::move(n)) {}

    std::string nombre;
    std::string categoria;  // "propio", "librería", "procedural" o "grupo" (para la interfaz)
    glm::vec3 posicion{0.0f};
    glm::vec3 rotacion{0.0f};  // grados, orden X -> Y -> Z
    glm::vec3 escala{1.0f};

    const Modelo* modelo = nullptr;  // modelo al que pertenece la malla
    int malla = -1;                  // índice de malla dibujada en este nodo
    Lote* lote = nullptr;            // instancias asociadas (opcional)
    bool visible = true;
    glm::vec4 tinte{1.0f};           // color multiplicativo que heredan los hijos
    bool raizInstancia = false;      // nodo de colocación de un modelo (destino de la selección con el ratón)

    Nodo* padre = nullptr;
    std::vector<std::unique_ptr<Nodo>> hijos;
    glm::mat4 local{1.0f};
    glm::mat4 mundo{1.0f};

    Nodo* agregar(std::unique_ptr<Nodo> hijo);
    Nodo* agregar(const std::string& nombreHijo);
    Nodo* buscar(const std::string& nombreBuscado);

    void colocar(const glm::vec3& p, const glm::vec3& rotGrados = glm::vec3(0.0f), const glm::vec3& esc = glm::vec3(1.0f));
    glm::mat4 matrizLocal() const;
    void actualizar(const glm::mat4& mundoPadre);
    int contarNodos() const;

};
