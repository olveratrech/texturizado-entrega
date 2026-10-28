#include "render/Renderer.h"

#include "Registro.h"
#include "Rutas.h"

#include <glm/gtc/matrix_inverse.hpp>

#include <algorithm>
#include <cmath>

bool Renderer::iniciar() {
    bool ok = shader_.cargar(rutas::shader("escena.vert"), rutas::shader("escena.frag"));
    ok = ok && shaderInst_.cargar(rutas::shader("escena.vert"), rutas::shader("escena.frag"), "#define INSTANCIADO\n");
    return ok;
}

void Renderer::liberar() {
    shader_.liberar();
    shaderInst_.liberar();

}

// Extrae los 6 planos del volumen de visión de la matriz vista-proyección
// (método de Gribb y Hartmann).
static void extraerPlanos(const glm::mat4& m, glm::vec4 p[6]) {
    glm::vec4 f0(m[0][0], m[1][0], m[2][0], m[3][0]);
    glm::vec4 f1(m[0][1], m[1][1], m[2][1], m[3][1]);
    glm::vec4 f2(m[0][2], m[1][2], m[2][2], m[3][2]);
    glm::vec4 f3(m[0][3], m[1][3], m[2][3], m[3][3]);
    p[0] = f3 + f0;
    p[1] = f3 - f0;
    p[2] = f3 + f1;
    p[3] = f3 - f1;
    p[4] = f3 + f2;
    p[5] = f3 - f2;
}

bool Renderer::visible(const glm::vec3& c, float r) const {
    if (!descartar) return true;
    for (const Plano& p : frustum_)
        if (glm::dot(p.n, c) + p.d < -r) return false;
    return true;
}

static float escalaMaxima(const glm::mat4& m) {
    return std::max({glm::length(glm::vec3(m[0])), glm::length(glm::vec3(m[1])), glm::length(glm::vec3(m[2]))});
}

void Renderer::recolectar(Nodo* n) {
    if (!n->visible) return;
    if (n->modelo && n->malla >= 0) {
        const Malla& malla = n->modelo->mallas[static_cast<size_t>(n->malla)];
        float esc = escalaMaxima(n->mundo);
        for (const Primitiva& p : malla.primitivas) {
            glm::vec3 c = glm::vec3(n->mundo * glm::vec4((p.min + p.max) * 0.5f, 1.0f));
            float r = glm::length(p.max - p.min) * 0.5f * esc;
            if (!visible(c, r)) {
                stats.descartados++;
                continue;
            }
            Dibujo d;
            d.prim = &p;
            d.mat = &n->modelo->materiales[static_cast<size_t>(p.material)];
            d.matriz = n->mundo;


            opacos_.push_back(d);
        }
    }
    if (n->lote && n->lote->instancias() > 0) {
        const Lote& L = *n->lote;
        glm::vec3 c = glm::vec3(n->mundo * glm::vec4((L.min + L.max) * 0.5f, 1.0f));
        float r = glm::length(L.max - L.min) * 0.5f * escalaMaxima(n->mundo);
        if (visible(c, r)) {
            for (size_t e = 0; e < L.modelo->plano.size(); ++e) {
                const auto& entrada = L.modelo->plano[e];
                const Malla& malla = L.modelo->mallas[static_cast<size_t>(entrada.malla)];
                for (size_t k = 0; k < malla.primitivas.size(); ++k) {
                    Dibujo d;
                    d.prim = &malla.primitivas[k];
                    d.mat = &L.modelo->materiales[static_cast<size_t>(d.prim->material)];
                    d.matriz = entrada.matriz;
                    d.padre = n->mundo;
                    d.lote = &L;
                    d.vaoInstancias = L.vaosDe(e)[k];


                    opacos_.push_back(d);
                }
            }
        } else {
            stats.descartados += static_cast<int>(L.modelo->plano.size());
        }
    }
    for (auto& h : n->hijos) recolectar(h.get());
}

void Renderer::aplicarMaterial(const Shader&, const Material& m) {
    if (m.dobleCara || alambre) glDisable(GL_CULL_FACE);
    else glEnable(GL_CULL_FACE);
}

void Renderer::emitir(const Dibujo& d) {
    const Shader& s = d.lote ? shaderInst_ : shader_;
    aplicarMaterial(s, *d.mat);
    if (d.lote) {
        s.set("uPadre", d.padre);
        s.set("uModelo", d.matriz);
        glBindVertexArray(d.vaoInstancias);
        glDrawElementsInstanced(GL_TRIANGLES, d.prim->indices, d.prim->tipoIndice, nullptr, d.lote->instancias());
        stats.instancias += d.lote->instancias();
        stats.triangulos += d.prim->indices / 3 * d.lote->instancias();
    } else {
        s.set("uModelo", d.matriz);
        s.set("uNormal", glm::inverseTranspose(glm::mat3(d.matriz)));
        glBindVertexArray(d.prim->vao);
        glDrawElements(GL_TRIANGLES, d.prim->indices, d.prim->tipoIndice, nullptr);
        stats.triangulos += d.prim->indices / 3;
    }
    stats.llamadas++;
}

void Renderer::dibujar(Nodo* raiz, const Camara& camara, int ancho, int alto) {
    stats = Estadisticas{};
    float aspecto = alto > 0 ? static_cast<float>(ancho) / static_cast<float>(alto) : 1.0f;
    glm::mat4 vista = camara.vista();
    glm::mat4 proy = camara.proyeccion(aspecto);
    vistaProy_ = proy * vista;

    glm::vec4 planos[6];
    extraerPlanos(vistaProy_, planos);
    for (int i = 0; i < 6; ++i) {
        float l = glm::length(glm::vec3(planos[i]));
        frustum_[i] = {glm::vec3(planos[i]) / l, planos[i].w / l};
    }

    opacos_.clear();
    recolectar(raiz);
    glViewport(0, 0, ancho, alto);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    if (alambre) {
        glClearColor(0.07f, 0.07f, 0.07f, 1.0f);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glClearColor(0.16f, 0.16f, 0.16f, 1.0f);
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    for (const Shader* s : {&shader_, &shaderInst_}) {
        s->usar();
        s->set("uVistaProy", vistaProy_);
        s->set("uAlambre", alambre ? 1 : 0);
    }

    const Shader* actual = nullptr;
    for (const Dibujo& d : opacos_) {
        const Shader* s = d.lote ? &shaderInst_ : &shader_;
        if (s != actual) {
            s->usar();
            actual = s;
        }
        emitir(d);
    }

    if (alambre) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBindVertexArray(0);
    glEnable(GL_CULL_FACE);
}
