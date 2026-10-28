#version 330 core
// Vértices de la escena. Con INSTANCIADO la matriz de cada copia llega como
// atributo (divisor 1) y una sola llamada dibuja miles de pétalos o veladoras.

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
#ifdef INSTANCIADO
layout(location = 3) in mat4 aInstancia;  // ocupa las ubicaciones 3, 4, 5 y 6
uniform mat4 uPadre;  // matriz de mundo del nodo dueño del lote
#endif

uniform mat4 uModelo;    // sin instancias: matriz de mundo; con instancias: matriz del nodo dentro del modelo
uniform mat3 uNormal;    // inversa transpuesta de uModelo (escala no uniforme)
uniform mat4 uVistaProy;

out vec3 vNormal;

void main() {
#ifdef INSTANCIADO
    mat4 M = uPadre * aInstancia * uModelo;
    vNormal = mat3(M) * aNormal;  // las instancias usan escala uniforme
#else
    mat4 M = uModelo;
    vNormal = uNormal * aNormal;
#endif
    vec4 p = M * vec4(aPos, 1.0);
    gl_Position = uVistaProy * p;
}
