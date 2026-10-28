#include "App.h"

#include "Registro.h"
#include "Rutas.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>


static App* g_app = nullptr;

int App::ejecutar(const Opciones& opciones) {
    op_ = opciones;
    if (!iniciar()) {
        terminar();
        return 1;
    }

    int cuadros=0;
    double anterior = glfwGetTime();
    while (!glfwWindowShouldClose(ventana_)) {
        double ahora = glfwGetTime();
        float dt = static_cast<float>(std::min(ahora - anterior, 0.1));
        anterior = ahora;
        glfwPollEvents();
        glfwGetFramebufferSize(ventana_, &anchoFb_, &altoFb_);
        if (anchoFb_ == 0 || altoFb_ == 0) {  // ventana minimizada
            glfwWaitEvents();
            continue;
        }
        procesarEntrada(dt);

        renderer_.dibujar(escena_.raiz(), camara_, anchoFb_, altoFb_);

        glfwSwapBuffers(ventana_);
        if(op_.comprobar && ++cuadros==5) {
            const GLenum error=glGetError();
            registro::info("COMPROBACION GL: vista=%d, llamadas=%d, triangulos=%d, error=%u",op_.vistaInicial,renderer_.stats.llamadas,renderer_.stats.triangulos,error);
            terminar();return error==GL_NO_ERROR?0:2;
        }
    }
    terminar();
    return 0;
}

bool App::iniciar() {
    g_app = this;
    glfwSetErrorCallback([](int codigo, const char* desc) { registro::error("GLFW %d: %s", codigo, desc); });
    if (!glfwInit()) {
        registro::fatal("No se pudo iniciar GLFW.");
        return false;
    }
    if(op_.comprobar)glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    if (op_.msaa > 0) glfwWindowHint(GLFW_SAMPLES, op_.msaa);
    if (op_.ancho <= 0 || op_.alto <= 0) {
        // tamaño inicial: 80 % del área de trabajo del monitor principal
        int x = 0, y = 0, w = 1280, h = 720;
        if (GLFWmonitor* mon = glfwGetPrimaryMonitor()) glfwGetMonitorWorkarea(mon, &x, &y, &w, &h);
        op_.ancho = std::max(960, w * 4 / 5);
        op_.alto = std::max(540, h * 4 / 5);
    }
    ventana_ = glfwCreateWindow(op_.ancho, op_.alto, "Cempasúchil | Texturizado",
                               nullptr, nullptr);
    if (!ventana_) {
        registro::fatal("No se pudo crear la ventana. Se requiere una tarjeta gráfica compatible con OpenGL 3.3.");
        return false;
    }
    glfwMakeContextCurrent(ventana_);
    glfwSwapInterval(1);  // sincronía vertical: evita trabajo de más en la GPU
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        registro::fatal("No se pudieron cargar las funciones de OpenGL.");
        return false;
    }
    registro::info("OpenGL %s | GLSL %s | %s", glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION),
                   glGetString(GL_RENDERER));
    if (op_.msaa > 0) glEnable(GL_MULTISAMPLE);

    if (!renderer_.iniciar()) {
        registro::fatal("No se pudieron compilar los shaders (carpeta shaders/).");
        return false;
    }
    if (!escena_.cargar()) {
        registro::fatal("No se pudieron cargar los modelos (carpeta assets/).");
        return false;
    }
    glfwSetKeyCallback(ventana_, alTeclear);
    glfwSetScrollCallback(ventana_, alDesplazar);
    glfwSetMouseButtonCallback(ventana_, alPulsar);
    aplicarVista(op_.vistaInicial);
    return true;
}

void App::terminar() {
    escena_.liberar();
    renderer_.liberar();
    if (ventana_) glfwDestroyWindow(ventana_);
    ventana_ = nullptr;
    glfwTerminate();
}

// ---------------------------------------------------------------------------
// Entrada
// ---------------------------------------------------------------------------
// Navegacion libre: no hay seleccion ni edicion de objetos.
void App::procesarEntrada(float dt) {
    double x, y;
    glfwGetCursorPos(ventana_, &x, &y);
    float dx = float(x - ultimoX_), dy = float(y - ultimoY_);
    ultimoX_ = x; ultimoY_ = y;
    if (arrastrando_) camara_.girar(dx, dy);
    auto tecla = [&](int k) { return glfwGetKey(ventana_, k) == GLFW_PRESS; };
    glm::vec3 d(0.0f);
    if (tecla(GLFW_KEY_W)) d.z += 1;
    if (tecla(GLFW_KEY_S)) d.z -= 1;
    if (tecla(GLFW_KEY_D)) d.x += 1;
    if (tecla(GLFW_KEY_A)) d.x -= 1;
    if (tecla(GLFW_KEY_E)) d.y += 1;
    if (tecla(GLFW_KEY_Q)) d.y -= 1;
    if (d != glm::vec3(0)) camara_.desplazar(glm::normalize(d), dt, tecla(GLFW_KEY_LEFT_SHIFT) ? 3.0f : 1.0f);
}

void App::alTeclear(GLFWwindow* v, int tecla, int codigo, int accion, int mods) {
    if (accion != GLFW_PRESS) return;
    App& a = *g_app;
    switch (tecla) {
        case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(v, GLFW_TRUE); break;
        case GLFW_KEY_F2: a.renderer_.alambre = !a.renderer_.alambre; break;
        case GLFW_KEY_R: a.aplicarVista(0); break;
        default: break;
    }
}

void App::alDesplazar(GLFWwindow* v, double dx, double dy) {
    g_app->camara_.acercar(float(dy));
}

void App::alPulsar(GLFWwindow* v, int boton, int accion, int mods) {
    if (boton != GLFW_MOUSE_BUTTON_RIGHT) return;
    App& a = *g_app;
    if (accion == GLFW_PRESS) {
        a.arrastrando_ = true;
        glfwSetInputMode(v, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwGetCursorPos(v, &a.ultimoX_, &a.ultimoY_);
    } else if (accion == GLFW_RELEASE) {
        a.arrastrando_ = false;
        glfwSetInputMode(v, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

void App::aplicarVista(int indice) {
    const auto& vistas = escena_.vistas();
    if (indice < 0 || indice >= static_cast<int>(vistas.size())) return;
    camara_.posicion = vistas[static_cast<size_t>(indice)].posicion;
    camara_.mirarHacia(vistas[static_cast<size_t>(indice)].objetivo);
}
