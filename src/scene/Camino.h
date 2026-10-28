// Camino.h - Trayectoria del camino de cempasúchil.
//
// La ruta se define con pocos puntos de control y se interpola con una
// spline Catmull-Rom centrípeta (pasa por todos los puntos y no forma lazos).
// Se re-muestrea por longitud de arco para poder colocar objetos cada
// cierta distancia (pétalos, veladoras, faroles, papel picado).
#pragma once

#include "render/Model.h"

#include <glm/glm.hpp>

#include <vector>

class Camino {
public:
    void construir(const std::vector<glm::vec3>& control, int muestrasPorTramo = 24);

    float longitud() const { return s_.empty() ? 0.0f : s_.back(); }
    glm::vec3 punto(float s) const;
    glm::vec3 tangente(float s) const;
    glm::vec3 izquierda(float s) const;  // perpendicular horizontal hacia la izquierda del avance

    // Franja de empedrado (malla procedural) siguiendo la curva.
    void malla(float ancho, float escalaUV, float altura, std::vector<Vertice>& vs, std::vector<uint32_t>& idx) const;

    const std::vector<glm::vec3>& control() const { return control_; }
    const std::vector<glm::vec3>& muestras() const { return muestras_; }

private:
    std::vector<glm::vec3> control_;
    std::vector<glm::vec3> muestras_;
    std::vector<float> s_;  // longitud acumulada en cada muestra
};
