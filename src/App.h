// Aplicación independiente para la entrega de texturizado.
#pragma once
#include "render/Renderer.h"
#include "scene/Camera.h"
#include "scene/Escena.h"
#include <string>
struct GLFWwindow;
struct Opciones {
    int ancho=0, alto=0, msaa=4;
    bool comprobar=false;
    int vistaInicial=0;
};
class App {
public:
    int ejecutar(const Opciones& opciones);
private:
    bool iniciar();
    void terminar();
    void procesarEntrada(float dt);
    void aplicarVista(int indice);
    static void alTeclear(GLFWwindow*,int,int,int,int);
    static void alDesplazar(GLFWwindow*,double,double);
    static void alPulsar(GLFWwindow*,int,int,int);
    GLFWwindow* ventana_=nullptr;
    Opciones op_;
    Escena escena_;
    Renderer renderer_;
    Camara camara_;
    bool arrastrando_=false;
    double ultimoX_=0, ultimoY_=0;
    int anchoFb_=1, altoFb_=1;
};
