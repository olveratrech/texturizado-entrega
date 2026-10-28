// Escena.h - Construcción del mundo "Camino de Cempasúchil".
//
// Distribución (vista superior, eje -Z = norte):
//
//          [ casa de adobe con portal ]  z = -16
//              altar + arco + ofrendas
//                    |
//      faroles  ~ camino de pétalos ~  papel picado      (spline Catmull-Rom)
//                    |
//          [ portada del panteón ]       z = +16
//        tumbas, criptas, cruz atrial, cipreses
//
// Los modelos se cargan una sola vez y se reutilizan: las piezas que se repiten
// muchas veces (pétalos, veladoras, flores, bardas) se dibujan instanciadas.
#pragma once

#include "render/Model.h"
#include "render/Skybox.h"
#include "scene/Camino.h"
#include "scene/SceneNode.h"

#include <glm/glm.hpp>

#include <map>
#include <memory>
#include <string>
#include <vector>

struct InfoModelo {
    std::string clave;        // nombre de archivo sin extensión
    std::string titulo;
    std::string descripcion;
    std::string origen;       // "Propio", "Librería" o "Procedural"
    Modelo* modelo = nullptr;
    int usos = 0;             // veces que aparece en la escena (incluye instancias)
};

struct Vista {
    std::string nombre;
    glm::vec3 posicion;
    glm::vec3 objetivo;
};

class Escena {
public:
    bool cargar();
    void liberar();

    Nodo* raiz() { return raiz_.get(); }
    Modelo* modelo(const std::string& clave);
    std::vector<InfoModelo>& catalogo() { return catalogo_; }
    const std::vector<Vista>& vistas() const { return vistas_; }

    // Crea un nodo de colocación y debajo replica la jerarquía de nodos del modelo.
    // contar = false para usos fuera de la escena (galería de modelos).
    Nodo* instanciar(const std::string& clave, Nodo* padre, const std::string& nombre, const glm::vec3& pos,
                     float rotY = 0.0f, float escala = 1.0f, bool contar = true);
    int totalInstancias() const;
    int totalModelosDistintos() const;

    Camino camino;
    Skybox cielo;

private:
    bool cargarModelos();
    void crearProcedurales();
    Lote* nuevoLote(const std::string& clave, Nodo* dueno);
    Nodo* nodoLote(const std::string& clave, Nodo* padre, const std::string& nombre);
    Nodo* crearTiraPapel(Nodo* padre, const std::string& nombre, const glm::vec3& posicion,
                         float giro, float ancho, int variante, int cantidad = 8);
    void construirCasaYOfrenda();
    void construirAltarAprobado(Nodo* altarAprobado);
    void construirCamino();
    void construirPanteon();
    void construirAlrededores();
    void replicar(const Modelo& m, int indice, Nodo* padre);
    InfoModelo* info(const std::string& clave);

    std::unique_ptr<Nodo> raiz_;
    std::map<std::string, std::unique_ptr<Modelo>> modelos_;
    std::vector<std::unique_ptr<Lote>> lotes_;
    std::vector<InfoModelo> catalogo_;
    std::vector<Vista> vistas_;
};
