#include "render/Renderer.h"

#include "Registro.h"
#include "Rutas.h"

#include <glm/gtc/matrix_inverse.hpp>

#include <algorithm>
#include <cctype>
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

float Renderer::visibilidadPantalla(const glm::vec3& posicion) const {
    const glm::vec4 clip = vistaProy_ * glm::vec4(posicion, 1.0f);
    if (clip.w <= 0.0f) return 0.0f;
    const glm::vec2 ndc = glm::abs(glm::vec2(clip) / clip.w);
    const float borde = std::max(ndc.x, ndc.y);
    if (borde <= 0.88f) return 1.0f;
    if (borde >= 1.25f) return 0.0f;
    const float t = (borde - 0.88f) / (1.25f - 0.88f);
    return 1.0f - t * t * (3.0f - 2.0f * t);
}

void Renderer::registrarLuz(const Material& material, const glm::vec3& posicion, ZonaLuz zona,
                            float factorVisibilidad) {
    std::string nombre = material.nombre;
    std::transform(nombre.begin(), nombre.end(), nombre.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    const bool esFarol = nombre.find("luz base") != std::string::npos;
    if (nombre.find("llama") == std::string::npos && !esFarol) return;
    if (esFarol) {
        const float distancia = glm::length(posicion - ojo_);
        if (distancia > 10.0f) {
            const float t = std::clamp((distancia - 10.0f) / 4.0f, 0.0f, 1.0f);
            factorVisibilidad *= 1.0f - t * t * (3.0f - 2.0f * t);
        }
    } else if (zona == ZonaLuz::Camino || zona == ZonaLuz::Cementerio) {
        const float distancia = glm::length(posicion - ojo_);
        if (distancia > 5.0f) {
            const float t = std::clamp((distancia - 5.0f) / 5.0f, 0.0f, 1.0f);
            factorVisibilidad *= 1.0f - t * t * (3.0f - 2.0f * t);
        }
    }
    if (factorVisibilidad <= 0.001f) return;

    const float maximo = std::max({material.emision.r, material.emision.g, material.emision.b, 0.001f});
    glm::vec3 tono = material.emision / maximo;
    // Las llamas blancas exportadas se convierten a temperatura cálida; el
    // farol conserva su tono naranja exportado.
    if (std::abs(tono.r - tono.g) < 0.08f && std::abs(tono.g - tono.b) < 0.08f)
        tono = {1.0f, 0.48f, 0.14f};
    LuzPuntual luz;
    luz.posicion = posicion;
    luz.farol = esFarol;
    luz.factorVisibilidad = factorVisibilidad;
    luz.zona = zona;
    luz.color = tono * (esFarol ? 2.10f : 2.0f);
    luz.lineal = esFarol ? 0.55f : 0.75f;
    luz.cuadratica = esFarol ? 0.55f : 2.20f;
    // La bombilla está a ~2.68 m: el alcance debe superar esa altura para
    // tocar el piso. El cono, no este radio, limita el diámetro de la huella.
    luz.radio2 = esFarol ? 12.25f : 9.0f;
    luz.distancia2 = glm::dot(posicion - ojo_, posicion - ojo_);
    luces_.push_back(luz);
}

void Renderer::agruparLucesAltar() {
    std::vector<LuzPuntual> individuales;
    std::vector<LuzPuntual> restantes;
    for (const LuzPuntual& luz : luces_) {
        if (!luz.farol && luz.zona == ZonaLuz::Altar) individuales.push_back(luz);
        else restantes.push_back(luz);
    }
    if (individuales.empty()) {
        luces_.swap(restantes);
        return;
    }

    glm::vec3 minimo(1e30f), maximo(-1e30f), promedio(0.0f);
    for (const LuzPuntual& luz : individuales) {
        minimo = glm::min(minimo, luz.posicion);
        maximo = glm::max(maximo, luz.posicion);
        promedio += luz.posicion;
    }
    promedio /= static_cast<float>(individuales.size());

    // Dos mitades por cada uno de los tres niveles del altar. La asignación
    // depende de la posición, no del orden de recorrido, por lo que no titila.
    struct Acumulado { glm::vec3 posicion{0.0f}; glm::vec3 color{0.0f}; int cantidad = 0; } grupos[6];
    const float rangoY = std::max(maximo.y - minimo.y, 0.001f);
    for (const LuzPuntual& luz : individuales) {
        const int nivel = std::clamp(static_cast<int>(3.0f * (luz.posicion.y - minimo.y) / rangoY), 0, 2);
        const int lado = luz.posicion.x < promedio.x ? 0 : 1;
        Acumulado& grupo = grupos[nivel * 2 + lado];
        grupo.posicion += luz.posicion;
        grupo.color += luz.color;
        grupo.cantidad++;
    }
    for (const Acumulado& grupo : grupos) {
        if (!grupo.cantidad) continue;
        LuzPuntual luz;
        luz.posicion = grupo.posicion / static_cast<float>(grupo.cantidad);
        luz.color = grupo.color / static_cast<float>(grupo.cantidad);
        luz.cantidad = grupo.cantidad;
        luz.zona = ZonaLuz::Altar;
        luz.lineal = 0.75f;
        luz.cuadratica = 2.20f;
        luz.radio2 = 9.0f;
        luz.distancia2 = glm::dot(luz.posicion - ojo_, luz.posicion - ojo_);
        restantes.push_back(luz);
    }

    // Relleno cálido y amplio desde la pared posterior, como rebote de los
    // arcos y las velas. Su baja intensidad evita una mancha circular visible.
    LuzPuntual relleno;
    relleno.posicion = {promedio.x, maximo.y + 0.45f, minimo.z - 0.45f};
    relleno.color = {0.30f, 0.115f, 0.035f};
    relleno.lineal = 0.35f;
    relleno.cuadratica = 0.18f;
    relleno.radio2 = 16.0f;
    relleno.relleno = true;
    relleno.zona = ZonaLuz::Altar;
    relleno.distancia2 = glm::dot(relleno.posicion - ojo_, relleno.posicion - ojo_);
    restantes.push_back(relleno);
    luces_.swap(restantes);
}

void Renderer::recolectar(Nodo* n, const glm::vec4& tintePadre, ZonaLuz zona) {
    if (!n->visible) return;
    if (n->nombre == "Altar aprobado") zona = ZonaLuz::Altar;
    else if (n->nombre == "Camino de cempasúchil") zona = ZonaLuz::Camino;
    else if (n->nombre == "Tumbas") zona = ZonaLuz::Cementerio;
    const glm::vec4 tinte = tintePadre * n->tinte;
    if (n->modelo && n->malla >= 0) {
        const Malla& malla = n->modelo->mallas[static_cast<size_t>(n->malla)];
        float esc = escalaMaxima(n->mundo);
        for (const Primitiva& p : malla.primitivas) {
            glm::vec3 c = glm::vec3(n->mundo * glm::vec4((p.min + p.max) * 0.5f, 1.0f));
            float r = glm::length(p.max - p.min) * 0.5f * esc;
            const Material& material = n->modelo->materiales[static_cast<size_t>(p.material)];
            const bool fuenteFarol = material.nombre.find("luz base") != std::string::npos;
            // Los faroles permanecen activos por proximidad aunque la cámara
            // mire al suelo o deje el foco fuera del encuadre. Su apagado
            // gradual se calcula por distancia dentro de registrarLuz().
            if (fuenteFarol) registrarLuz(material, c, zona, 1.0f);
            if (!visible(c, r)) {
                stats.descartados++;
                continue;
            }
            Dibujo d;
            d.prim = &p;
            d.mat = &material;
            if (!fuenteFarol) registrarLuz(*d.mat, c, zona);
            d.matriz = n->mundo;
            d.tinte = tinte;
            d.distancia2 = glm::dot(c - ojo_, c - ojo_);
            (d.mat->mezcla ? transparentes_ : opacos_).push_back(d);
        }
    }
    if (n->lote && n->lote->instancias() > 0) {
        const Lote& L = *n->lote;
        glm::vec3 c = glm::vec3(n->mundo * glm::vec4((L.min + L.max) * 0.5f, 1.0f));
        float r = glm::length(L.max - L.min) * 0.5f * escalaMaxima(n->mundo);
        // Las veladoras del camino mantienen sus luces aunque el lote completo
        // quede momentáneamente fuera del frustum.
        if (zona == ZonaLuz::Camino)
            for (const auto& entrada : L.modelo->plano) {
                const Malla& mallaLuz = L.modelo->mallas[static_cast<size_t>(entrada.malla)];
                for (const Primitiva& primitiva : mallaLuz.primitivas) {
                    const Material& material = L.modelo->materiales[static_cast<size_t>(primitiva.material)];
                    if (glm::dot(material.emision, material.emision) <= 0.01f) continue;
                    const glm::vec3 centroLocal = (primitiva.min + primitiva.max) * 0.5f;
                    for (const glm::mat4& instancia : L.transformaciones) {
                        const glm::mat4 mundo = n->mundo * instancia * entrada.matriz;
                        const glm::vec3 posicion = glm::vec3(mundo * glm::vec4(centroLocal, 1.0f));
                        registrarLuz(material, posicion, zona, 1.0f);
                    }
                }
            }
        if (visible(c, r)) {
            for (size_t e = 0; e < L.modelo->plano.size(); ++e) {
                const auto& entrada = L.modelo->plano[e];
                const Malla& malla = L.modelo->mallas[static_cast<size_t>(entrada.malla)];
                for (size_t k = 0; k < malla.primitivas.size(); ++k) {
                    Dibujo d;
                    d.prim = &malla.primitivas[k];
                    d.mat = &L.modelo->materiales[static_cast<size_t>(d.prim->material)];
                    if (zona != ZonaLuz::Camino && glm::dot(d.mat->emision, d.mat->emision) > 0.01f)
                        for (const glm::mat4& instancia : L.transformaciones) {
                            const glm::mat4 mundo = n->mundo * instancia * entrada.matriz;
                            const glm::vec3 centroLocal = (d.prim->min + d.prim->max) * 0.5f;
                            const glm::vec3 posicion = glm::vec3(mundo * glm::vec4(centroLocal, 1.0f));
                            registrarLuz(*d.mat, posicion, zona, 1.0f);
                        }
                    d.matriz = entrada.matriz;
                    d.padre = n->mundo;
                    d.lote = &L;
                    d.vaoInstancias = L.vaosDe(e)[k];
                    d.tinte = tinte;
                    d.distancia2 = glm::dot(c - ojo_, c - ojo_);
                    (d.mat->mezcla ? transparentes_ : opacos_).push_back(d);
                }
            }
        } else {
            stats.descartados += static_cast<int>(L.modelo->plano.size());
        }
    }
    for (auto& h : n->hijos) recolectar(h.get(), tinte, zona);
}

void Renderer::aplicarLuces(const Shader& s, const Camara& camara) const {
    constexpr int maxLuces = 24;
    const int cantidad = lucesPuntuales
        ? std::min<int>(maxLuces, static_cast<int>(luces_.size())) : 0;
    s.set("uCantidadLuces", cantidad);
    s.set("uDireccionLuna", glm::normalize(glm::vec3(-0.35f, 0.82f, 0.28f)));
    s.set("uColorLuna", glm::vec3(0.035f, 0.060f, 0.120f));
    s.set("uAmbienteNoche", glm::vec3(0.014f, 0.020f, 0.040f));
    for (int i = 0; i < cantidad; ++i) {
        const std::string base = "uLuces[" + std::to_string(i) + "]";
        s.set((base + ".posicion").c_str(), luces_[static_cast<size_t>(i)].posicion);
        const LuzPuntual& luz = luces_[static_cast<size_t>(i)];
        const float refuerzoGrupo = 1.0f + 0.06f * static_cast<float>(std::min(luz.cantidad - 1, 3));
        const float intensidadZona = luz.zona == ZonaLuz::Altar && !luz.relleno ? 0.22f : 1.0f;
        s.set((base + ".color").c_str(), luz.color * refuerzoGrupo * intensidadZona *
                                            luz.factorVisibilidad);
        s.set((base + ".lineal").c_str(), luz.lineal);
        s.set((base + ".cuadratica").c_str(), luz.cuadratica);
        s.set((base + ".radio2").c_str(), luz.radio2);
        s.set((base + ".farol").c_str(), luz.farol ? 1.0f : 0.0f);
    }
    s.set("uSpotActivo", spotlight ? 1 : 0);
    s.set("uSpotPosicion", camara.ojo());
    s.set("uSpotDireccion", glm::normalize(camara.frente()));
}

void Renderer::aplicarMaterial(const Shader& s, const Material& m) {
    if (m.dobleCara || alambre) glDisable(GL_CULL_FACE);
    else glEnable(GL_CULL_FACE);
    s.set("uColorBase", m.colorBase);
    std::string nombreMaterial = m.nombre;
    std::transform(nombreMaterial.begin(), nombreMaterial.end(), nombreMaterial.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    const float refuerzoEmision = nombreMaterial.find("llama") != std::string::npos ? 1.45f : 1.0f;
    s.set("uEmision", m.emision * refuerzoEmision);
    s.set("uMetalico", m.metalico);
    s.set("uRugosidad", m.rugosidad);
    s.set("uEscalaNormal", m.escalaNormal);
    s.set("uOclusion", m.oclusion);
    s.set("uEspecular", m.especular);
    s.set("uTransmision", m.transmision);
    s.set("uIor", m.ior);
    s.set("uTipoTransparente", m.tipoTransparente);
    s.set("uCorteAlfa", m.corteAlfa);
    s.set("uMascara", m.mascara ? 1 : 0);
    s.set("uSinLuz", m.sinLuz ? 1 : 0);
    s.set("uRecibeLucesPuntuales", 1);
    const GLuint ids[] = {m.texturaColor, m.texturaMetalRugosidad, m.texturaNormal,
                          m.texturaOclusion, m.texturaEmision};
    const char* presentes[] = {"uTieneColor", "uTieneMetalRugosidad", "uTieneNormal",
                               "uTieneOclusion", "uTieneEmision"};
    const char* samplers[] = {"uTexturaColor", "uTexturaMetalRugosidad", "uTexturaNormal",
                              "uTexturaOclusion", "uTexturaEmision"};
    for (int i = 0; i < 5; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, ids[i]);
        s.set(presentes[i], ids[i] ? 1 : 0);
        s.set(samplers[i], i);
    }
}

void Renderer::emitir(const Dibujo& d) {
    const Shader& s = d.lote ? shaderInst_ : shader_;
    aplicarMaterial(s, *d.mat);
    s.set("uTinte", d.tinte);
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

void Renderer::dibujar(Nodo* raiz, const Camara& camara, int ancho, int alto, const Skybox* cielo) {
    stats = Estadisticas{};
    float aspecto = alto > 0 ? static_cast<float>(ancho) / static_cast<float>(alto) : 1.0f;
    glm::mat4 vista = camara.vista();
    glm::mat4 proy = camara.proyeccion(aspecto);
    vistaProy_ = proy * vista;
    ojo_ = camara.ojo();

    glm::vec4 planos[6];
    extraerPlanos(vistaProy_, planos);
    for (int i = 0; i < 6; ++i) {
        float l = glm::length(glm::vec3(planos[i]));
        frustum_[i] = {glm::vec3(planos[i]) / l, planos[i].w / l};
    }

    opacos_.clear();
    transparentes_.clear();
    luces_.clear();
    recolectar(raiz);
    agruparLucesAltar();
    auto prioridad=[](const LuzPuntual& luz) {
        if (luz.farol) return 0;
        if (luz.zona == ZonaLuz::Camino) return 1;
        if (luz.zona == ZonaLuz::Altar) return 2;
        if (luz.zona == ZonaLuz::Otra) return 3;
        return 4; // cementerio: visible, pero prescindible si se alcanza el límite
    };
    std::sort(luces_.begin(), luces_.end(), [&](const LuzPuntual& a, const LuzPuntual& b) {
        const int pa=prioridad(a),pb=prioridad(b);
        if(pa!=pb)return pa<pb;
        return a.distancia2 < b.distancia2;
    });
    // Las fuentes más cercanas de cada zona aportan casi toda la luz visible.
    // Acotar la lista evita recorrer decenas de velas detrás de cada cristal.
    if (luces_.size() > 24) luces_.resize(24);
    stats.luces = static_cast<int>(luces_.size());
    std::stable_sort(transparentes_.begin(), transparentes_.end(),
                     [](const Dibujo& a, const Dibujo& b) { return a.distancia2 > b.distancia2; });
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
        s->set("uOjo", ojo_);
        aplicarLuces(*s, camara);
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

    if (!alambre && cielo) {
        cielo->dibujar(vista, proy);
        actual = nullptr; // el skybox dejó activo su propio programa GLSL
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    for (const Dibujo& d : transparentes_) {
        const Shader* s = d.lote ? &shaderInst_ : &shader_;
        if (s != actual) {
            s->usar();
            actual = s;
        }
        emitir(d);
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    if (alambre) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBindVertexArray(0);
    glEnable(GL_CULL_FACE);
}
