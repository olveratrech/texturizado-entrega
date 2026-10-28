// Model.h - Modelos 3D en formato glTF 2.0 (.gltf/.glb) cargados con cgltf.
//
// Un Modelo conserva la jerarquía de nodos del archivo (por ejemplo
// altar_muertos -> nivel_1 -> nivel_2 -> nivel_3) para que la escena pueda
// replicarla como nodos editables y, más adelante, animarla.
#pragma once


#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// Formato de vértice intercalado (32 bytes): posición, normal y coordenada UV.
struct Vertice {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct Material {
    std::string nombre = "material";
    bool dobleCara = false;
    bool mezcla = false;
    bool mascara = false;
    bool sinLuz = false;
    float corteAlfa = 0.5f;
    glm::vec4 colorBase{1.0f};
    glm::vec3 emision{0.0f};
    float metalico = 1.0f;
    float rugosidad = 1.0f;
    float escalaNormal = 1.0f;
    float oclusion = 1.0f;
    float especular = 1.0f;
    float transmision = 0.0f;
    GLuint texturaColor = 0;
    GLuint texturaMetalRugosidad = 0;
    GLuint texturaNormal = 0;
    GLuint texturaOclusion = 0;
    GLuint texturaEmision = 0;
};

struct Primitiva {
    GLuint vao = 0, vbo = 0, ebo = 0;
    GLsizei indices = 0;
    GLenum tipoIndice = GL_UNSIGNED_SHORT;  // 16 bits cuando hay menos de 65536 vértices
    int vertices = 0;
    int material = 0;
    glm::vec3 min{0.0f}, max{0.0f};          // caja envolvente local
};

struct Malla {
    std::string nombre;
    std::vector<Primitiva> primitivas;
    std::vector<glm::vec3> superficie; // geometría CPU de pan/dulces para verificar contacto
};

struct NodoModelo {
    std::string nombre;
    glm::vec3 traslacion{0.0f};
    glm::quat rotacion{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 escala{1.0f};
    int malla = -1;
    std::vector<int> hijos;
};

class Modelo {
public:
    inline static bool soloCPU=false;
    std::string nombre;
    std::string ruta;
    std::vector<Malla> mallas;
    std::vector<Material> materiales;
    std::vector<GLuint> texturas;
    std::vector<NodoModelo> nodos;
    std::vector<int> raices;

    // Nodos con malla "aplanados" (matriz acumulada dentro del modelo); se usa
    // para el dibujo instanciado, donde el modelo completo se repite N veces.
    struct Plano {
        int malla;
        glm::mat4 matriz;
    };
    std::vector<Plano> plano;

    int triangulos = 0;
    int vertices = 0;
    glm::vec3 min{0.0f}, max{0.0f};  // caja envolvente del modelo completo
    float baseY = 0.0f; // mínimo Y de los vértices reales en coordenadas del modelo
    float radio() const { return glm::length(max - min) * 0.5f; }
    glm::vec3 centro() const { return (min + max) * 0.5f; }

    bool cargar(const std::string& ruta);
    static std::unique_ptr<Modelo> procedural(const std::string& nombre, const std::vector<Vertice>& vertices,
                                              const std::vector<uint32_t>& indices, const Material& material);
    void liberar();
    ~Modelo() { liberar(); }

    static glm::mat4 matrizNodo(const NodoModelo& n);

private:
    void calcularPlano();
};

// Sube vértices e índices a la GPU y configura el VAO (atributos 0, 1 y 2).
Primitiva crearPrimitiva(const std::vector<Vertice>& vertices, const std::vector<uint32_t>& indices, int material);
void configurarAtributosVertice();
