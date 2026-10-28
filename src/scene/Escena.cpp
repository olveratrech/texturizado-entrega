#include "scene/Escena.h"

#include "Registro.h"
#include "Rutas.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cmath>
#include <random>
#include <functional>
#include <stdexcept>

namespace {

// Catálogo de modelos: clave de archivo, título, descripción y origen.
struct Ficha {
    const char* clave;
    const char* titulo;
    const char* descripcion;
    const char* carpeta = "";
};

const Ficha kPropios[] = {
    {"altar_muertos", "Altar de muertos", ""},
    {"arco_cempasuchil", "Arco de cempasúchil", ""},
    {"flor_cempasuchil", "Flor de cempasúchil", ""},
    {"petalo_cempasuchil", "Pétalo de cempasúchil", ""},
    {"veladora_1", "Veladora 1", "", "veladoras/"},
    {"veladora_2", "Veladora 2", "", "veladoras/"},
    {"veladora_3", "Veladora 3", "", "veladoras/"},
    {"veladora_4", "Veladora 4", "", "veladoras/"},
    {"cabeza_cempasuchil", "Cabeza de cempasúchil", "Selección existente sin tallo", "../derivados/"},
    {"cirio_1", "Cirio casi nuevo", "", "cirios/"},
    {"cirio_2", "Cirio poco consumido", "", "cirios/"},
    {"cirio_3", "Cirio medio", "", "cirios/"},
    {"cirio_4", "Cirio muy consumido", "", "cirios/"},
    {"cirio_5", "Cirio remanente", "", "cirios/"},
    {"pan_de_muerto", "Pan de muerto", ""},
    {"calaverita_azucar", "Calaverita de azúcar", ""},
    {"papel_1", "Papel picado 1", "", "papel_picado/"},
    {"papel_2", "Papel picado 2", "", "papel_picado/"},
    {"papel_3", "Papel picado 3", "", "papel_picado/"},
    {"papel_4", "Papel picado 4", "", "papel_picado/"},
    {"papel_5", "Papel picado 5", "", "papel_picado/"},
    {"papel_6", "Papel picado 6", "", "papel_picado/"},
    {"sahumerio", "Sahumerio (popoxcomitl)", ""},
    {"portarretrato_1", "Portarretrato 1", "", "portarretratos/"},
    {"portarretrato_2", "Portarretrato 2", "", "portarretratos/"},
    {"portarretrato_3", "Portarretrato 3", "", "portarretratos/"},
    {"jarron_talavera", "Jarrón de Talavera", ""},
    {"vaso_agua", "Vaso con agua", ""},
    {"maceta_cempasuchil", "Maceta con cempasúchil", ""},
    {"mariposa_monarca", "Mariposa monarca", ""},
    {"farol", "Farol de hojalata", ""},
    {"entrada_panteon", "Portada del panteón", ""},
    {"casa_adobe", "Casa de adobe con portal", ""},
    // Extras de la ofrenda, en la carpeta raíz de modelos propios.
    {"botella_corona", "Botella de Corona", ""},
    {"botella_victoria", "Botella de Victoria", ""},
    {"jarro_cafe", "Jarro de barro con café", ""},
    {"olla_barro", "Olla de barro con tapa", ""},
    {"jarra_agua", "Jarra con agua", ""},
    {"frutero", "Frutero", ""},
    {"plato_tamales", "Plato de tamales", "", "platos_comida/"},
    {"plato_mole", "Plato de mole", "", "platos_comida/"},
    {"plato_carnitas", "Plato de carnitas", "", "platos_comida/"},
    {"plato_tacos_dorados", "Plato de tacos dorados", "", "platos_comida/"},
    {"dulce_alegria", "Alegría de amaranto", "", "dulces/"},
    {"dulce_palanqueta", "Palanqueta", "", "dulces/"},
    {"dulce_cocada", "Cocada", "", "dulces/"},
    {"dulce_jamoncillo", "Jamoncillo con nuez", "", "dulces/"},
    {"dulce_limon_relleno", "Limón relleno de coco", "", "dulces/"},
    {"dulce_tamarindo", "Dulce de tamarindo", "", "dulces/"},
    {"dulce_leche_estriado", "Dulce de leche estriado", "", "dulces/"},
};

const Ficha kLibreria[] = {
    {"grave", "Tumba de tierra", "Kenney Graveyard Kit (CC0)."},
    {"grave-border", "Tumba con bordillo", "Kenney Graveyard Kit (CC0)."},
    {"gravestone-cross", "Lápida con cruz", "Kenney Graveyard Kit (CC0)."},
    {"gravestone-round", "Lápida redonda", "Kenney Graveyard Kit (CC0)."},
    {"gravestone-bevel", "Lápida biselada", "Kenney Graveyard Kit (CC0)."},
    {"gravestone-decorative", "Lápida decorativa", "Kenney Graveyard Kit (CC0)."},
    {"cross-wood", "Cruz de madera", "Kenney Graveyard Kit (CC0)."},
    {"cross-column", "Cruz atrial", "Kenney Graveyard Kit (CC0)."},
    {"crypt", "Cripta", "Kenney Graveyard Kit (CC0)."},
    {"iron-fence", "Reja de hierro", "Kenney Graveyard Kit (CC0)."},
    {"stone-wall", "Barda de piedra", "Kenney Graveyard Kit (CC0)."},
    {"stone-wall-column", "Columna de barda", "Kenney Graveyard Kit (CC0)."},
    {"pine", "Ciprés", "Kenney Graveyard Kit (CC0)."},
    {"pine-crooked", "Ciprés torcido", "Kenney Graveyard Kit (CC0)."},
    {"bench", "Banca", "Kenney Graveyard Kit (CC0)."},
    {"urn-round", "Urna funeraria", "Kenney Graveyard Kit (CC0)."},
    {"pumpkin", "Calabaza de Castilla", "Kenney Graveyard Kit (CC0). Ingrediente de la calabaza en tacha."},
    {"pumpkin-tall", "Calabaza alargada", "Kenney Graveyard Kit (CC0)."},
    {"rocks", "Rocas", "Kenney Graveyard Kit (CC0)."},
    {"candle-multiple", "Velas", "Kenney Graveyard Kit (CC0)."},
    {"detail-bowl", "Cazuela", "Kenney Graveyard Kit (CC0). Representa la cazuela de mole."},
    {"detail-plate", "Plato", "Kenney Graveyard Kit (CC0)."},
    {"character-skeleton", "Esqueleto (calaca)", "Kenney Graveyard Kit (CC0). Personaje jerárquico de 8 nodos."},
    {"character-ghost", "Ánima", "Kenney Graveyard Kit (CC0). Personaje jerárquico."},
    {"character-keeper", "Panteonero", "Kenney Graveyard Kit (CC0). Personaje jerárquico de 8 nodos."},
};

// Generador pseudoaleatorio con semilla fija: la escena es idéntica en cada ejecución.
struct Azar {
    std::mt19937 gen{20261102u};
    float operator()(float a, float b) { return std::uniform_real_distribution<float>(a, b)(gen); }
    float normal(float sigma) { return std::normal_distribution<float>(0.0f, sigma)(gen); }
};

glm::mat4 trs(const glm::vec3& p, float rotYgrados, float escala = 1.0f) {
    return glm::translate(glm::mat4(1.0f), p) * glm::rotate(glm::mat4(1.0f), glm::radians(rotYgrados), {0, 1, 0}) *
           glm::scale(glm::mat4(1.0f), glm::vec3(escala));
}

// Ángulo (grados) que alinea el eje local +X con la dirección horizontal d.
float alinearX(const glm::vec3& d) { return glm::degrees(std::atan2(-d.z, d.x)); }
// Ángulo (grados) que alinea el eje local +Z (frente de los modelos) con d.
float alinearZ(const glm::vec3& d) { return glm::degrees(std::atan2(d.x, d.z)); }

}  // namespace

// ---------------------------------------------------------------------------
// Carga
// ---------------------------------------------------------------------------
bool Escena::cargar() {
    if (!cargarModelos()) return false;
    crearProcedurales();

    raiz_ = std::make_unique<Nodo>("Escena");
    raiz_->categoria = "grupo";
    Nodo* terreno = raiz_->agregar("Terreno");
    Nodo* suelo = terreno->agregar("suelo");
    suelo->modelo = modelo("terreno");
    suelo->malla = 0;
    suelo->categoria = "procedural";
    info("terreno")->usos++;

    try { construirCasaYOfrenda(); }
    catch(const std::exception& e){registro::error("Altar: %s",e.what());return false;}
    construirCamino();
    construirPanteon();
    construirAlrededores();

    for (auto& l : lotes_) l->subir();
    raiz_->actualizar(glm::mat4(1.0f));

    const glm::vec3 alto(0.0f, 1.0f, 0.0f);
    vistas_ = {
        {"Vista general", {-17.0f, 12.0f, 26.0f}, {0.0f, 0.0f, 2.0f}},
        {"Entrada del panteón", {1.2f, 1.7f, 8.5f}, {0.0f, 2.2f, 16.0f}},
        {"Camino de pétalos", camino.punto(3.0f) + camino.izquierda(3.0f) * 0.8f + alto * 1.5f,
         camino.punto(14.0f) + alto * 0.5f},
        {"Altar de muertos", {0.5f, 1.6f, -10.0f}, {0.0f, 1.15f, -15.2f}},
        {"Ofrenda (detalle)", {1.0f, 1.35f, -12.4f}, {0.0f, 0.95f, -15.1f}},
        {"Panteón", {-11.0f, 4.5f, 15.0f}, {0.0f, 0.5f, 26.0f}},
        {"Vista aérea", {0.0f, 60.0f, 21.0f}, {0.0f, 0.0f, 6.0f}},
    };
    for (const auto& i : catalogo_)
        if (i.usos == 0) registro::info("Aviso: el modelo %s no se usa en la escena", i.clave.c_str());
    registro::info("Escena lista: %d nodos, %d modelos, %d instancias",
                   raiz_->contarNodos(), totalModelosDistintos(), totalInstancias());
    return true;
}

bool Escena::cargarModelos() {
    auto cargarUno = [&](const std::string& clave, const std::string& ruta, const Ficha& f, const char* origen) {
        auto m = std::make_unique<Modelo>();
        if (!m->cargar(ruta)) return false;
        catalogo_.push_back({clave, f.titulo, f.descripcion, origen, m.get(), 0});
        modelos_[clave] = std::move(m);
        return true;
    };
    for (const Ficha& f : kPropios)
        if (!cargarUno(f.clave, rutas::asset(std::string("models/propios/") + f.carpeta + f.clave + ".gltf"), f, "Propio")) return false;
    // La cabeza suelta conserva su geometría derivada, pero reutiliza el
    // material de pétalos del modelo completo para no duplicar otra exportación.
    if (Modelo* cabeza = modelo("cabeza_cempasuchil")) {
        if (Modelo* flor = modelo("flor_cempasuchil")) {
            auto fuente = std::find_if(flor->materiales.begin(), flor->materiales.end(), [](const Material& m) {
                return m.nombre == "Petalo_Cempasuchil";
            });
            auto destino = std::find_if(cabeza->materiales.begin(), cabeza->materiales.end(), [](const Material& m) {
                return m.nombre == "Petalo_Cempasuchil";
            });
            if (fuente != flor->materiales.end() && destino != cabeza->materiales.end()) *destino = *fuente;
        }
    }
    for (const Ficha& f : kLibreria)
        if (!cargarUno(f.clave, rutas::asset(std::string("models/libreria/kenney_graveyard/") + f.clave + ".glb"), f,
                       "Librería"))
            return false;
    return true;
}

void Escena::crearProcedurales() {
    // Terreno: un solo cuadrilátero de 200 x 200 m (2 triángulos) con la textura repetida.
    {
        const float m = 100.0f;
        std::vector<Vertice> vs = {{{-m, 0, -m}, {0, 1, 0}, {-m, -m}},
                                   {{m, 0, -m}, {0, 1, 0}, {m, -m}},
                                   {{m, 0, m}, {0, 1, 0}, {m, m}},
                                   {{-m, 0, m}, {0, 1, 0}, {-m, m}}};
        std::vector<uint32_t> idx = {0, 2, 1, 0, 3, 2};
        Material mat;
        mat.nombre = "tierra";



        modelos_["terreno"] = Modelo::procedural("terreno", vs, idx, mat);
        catalogo_.push_back({"terreno", "Terreno", "Plano de 200 x 200 m con textura repetida (2 triángulos).",
                             "Procedural", modelos_["terreno"].get(), 0});
    }

    // Hilo de tres metros con caída suave; se reutiliza en todas las tiras.
    {
        std::vector<Vertice> vs;
        std::vector<uint32_t> indices;
        constexpr int tramos = 64, lados = 8;
        constexpr float pi = 3.14159265f;
        for (int j = 0; j <= tramos; ++j) {
            float x = -1.5f + 3.0f * j / tramos;
            float y = 0.18f * (x * x / 2.25f - 1.0f);
            glm::vec3 arriba = glm::normalize(glm::vec3(-0.16f * x, 1.0f, 0.0f));
            for (int k = 0; k < lados; ++k) {
                float a = 2.0f * pi * k / lados;
                glm::vec3 normal = arriba * std::cos(a) + glm::vec3(0, 0, std::sin(a));
                vs.push_back({glm::vec3(x, y, 0) + normal * 0.003f, normal, {0, 0}});
            }
        }
        for (int j = 0; j < tramos; ++j)
            for (int k = 0; k < lados; ++k) {
                uint32_t a = j * lados + k, b = j * lados + (k + 1) % lados;
                indices.insert(indices.end(), {a, b, b + lados, a, b + lados, a + lados});
            }
        Material mat;
        mat.nombre = "hilo";
        modelos_["hilo_papel"] = Modelo::procedural("hilo_papel", vs, indices, mat);
        catalogo_.push_back({"hilo_papel", "Hilo de papel picado", "", "Procedural", modelos_["hilo_papel"].get(), 0});
    }

    // Plano unitario para cubrir cada nivel del altar sin modificar su modelo exportado.
    {
        std::vector<Vertice> vs = {{{-0.5f, 0, -0.5f}, {0, 1, 0}, {0, 0}},
                                   {{ 0.5f, 0, -0.5f}, {0, 1, 0}, {1, 0}},
                                   {{ 0.5f, 0,  0.5f}, {0, 1, 0}, {1, 1}},
                                   {{-0.5f, 0,  0.5f}, {0, 1, 0}, {0, 1}}};
        std::vector<uint32_t> idx = {0, 2, 1, 0, 3, 2};
        Material mat;
        mat.nombre = "cubierta_mate";
        mat.metalico = 0.0f;
        mat.rugosidad = 0.86f;
        modelos_["cubierta_piso"] = Modelo::procedural("cubierta_piso", vs, idx, mat);
        catalogo_.push_back({"cubierta_piso", "Cubiertas de los pisos", "Planos de color sobre los tres niveles.",
                             "Procedural", modelos_["cubierta_piso"].get(), 0});
    }

}

Modelo* Escena::modelo(const std::string& clave) {
    auto it = modelos_.find(clave);
    return it == modelos_.end() ? nullptr : it->second.get();
}

InfoModelo* Escena::info(const std::string& clave) {
    for (auto& i : catalogo_)
        if (i.clave == clave) return &i;
    return nullptr;
}

int Escena::totalInstancias() const {
    int total = 0;
    for (const auto& i : catalogo_) total += i.usos;
    return total;
}

int Escena::totalModelosDistintos() const {
    int n = 0;
    for (const auto& i : catalogo_)
        if (i.usos > 0) ++n;
    return n;
}

void Escena::liberar() {
    raiz_.reset();
    lotes_.clear();
    modelos_.clear();
}

// ---------------------------------------------------------------------------
// Instancias y lotes
// ---------------------------------------------------------------------------
void Escena::replicar(const Modelo& m, int indice, Nodo* padre) {
    const NodoModelo& nm = m.nodos[static_cast<size_t>(indice)];
    auto n = std::make_unique<Nodo>(nm.nombre);
    n->posicion = nm.traslacion;
    n->rotacion = glm::degrees(glm::eulerAngles(nm.rotacion));
    n->escala = nm.escala;
    n->modelo = &m;
    n->malla = nm.malla;
    n->categoria = padre->categoria;
    Nodo* ptr = padre->agregar(std::move(n));
    for (int h : nm.hijos) replicar(m, h, ptr);
}

Nodo* Escena::instanciar(const std::string& clave, Nodo* padre, const std::string& nombre, const glm::vec3& pos,
                         float rotY, float escala, bool contar) {
    Modelo* m = modelo(clave);
    if (!m) {
        registro::error("Modelo desconocido: %s", clave.c_str());
        return padre;
    }
    auto n = std::make_unique<Nodo>(nombre);
    InfoModelo* i = info(clave);
    n->categoria = i ? (i->origen == "Propio" ? "propio" : i->origen == "Librería" ? "librería" : "procedural") : "grupo";
    n->colocar(pos, glm::vec3(0.0f, rotY, 0.0f), glm::vec3(escala));
    n->raizInstancia = true;
    Nodo* ptr = padre->agregar(std::move(n));
    for (int r : m->raices) replicar(*m, r, ptr);
    if (i && contar) i->usos++;
    return ptr;
}

Lote* Escena::nuevoLote(const std::string& clave, Nodo* dueno) {
    auto l = std::make_unique<Lote>();
    l->modelo = modelo(clave);
    dueno->lote = l.get();
    lotes_.push_back(std::move(l));
    return lotes_.back().get();
}

Nodo* Escena::nodoLote(const std::string& clave, Nodo* padre, const std::string& nombre) {
    Nodo* n = padre->agregar(nombre);
    InfoModelo* i = info(clave);
    n->categoria = i && i->origen == "Propio" ? "propio" : "librería";
    nuevoLote(clave, n);
    return n;
}

// ---------------------------------------------------------------------------
// Casa con portal y ofrenda
// ---------------------------------------------------------------------------
// Una tira es un grupo: hilo y ocho papeles con pivote en su borde superior.
Nodo* Escena::crearTiraPapel(Nodo* padre, const std::string& nombre,
                            const glm::vec3& posicion, float giro, float ancho, int variante) {
    Nodo* tira = padre->agregar(nombre);
    tira->colocar(posicion, {0.0f, giro, 0.0f});
    Nodo* hilo = instanciar("hilo_papel", tira, "hilo", {0, 0, 0});
    hilo->escala.x = ancho / 3.0f;
    constexpr int cantidad = 8;
    const std::array<glm::vec4,7> colores{{
        {0.82f,0.05f,0.34f,1}, {0.42f,0.08f,0.72f,1}, {1.00f,0.28f,0.03f,1},
        {0.03f,0.38f,0.88f,1}, {0.08f,0.62f,0.24f,1}, {0.96f,0.70f,0.03f,1},
        {0.88f,0.06f,0.08f,1}
    }};
    float paso = ancho / cantidad;
    for (int i = 0; i < cantidad; ++i) {
        std::string clave = "papel_" + std::to_string(1 + (i + variante) % 6);
        const Modelo* m = modelo(clave);
        float x = -ancho * 0.5f + paso * (i + 0.5f);
        float y = 0.18f * (4.0f * x * x / (ancho * ancho) - 1.0f);
        float pendiente = 1.44f * x / (ancho * ancho);
        Nodo* soporte = tira->agregar("pliego_" + std::to_string(i + 1));
        soporte->colocar({x, y, 0}, {0, 0, glm::degrees(std::atan(pendiente))});
        float escala = paso * 0.85f / (m->max.x - m->min.x);
        Nodo* pliego = instanciar(clave, soporte, clave,
                                  {-0.5f * (m->min.x + m->max.x) * escala, -m->max.y * escala, 0},
                                  0.0f, escala);
        pliego->tinte = colores[static_cast<size_t>((i + variante * 3) % colores.size())];
    }
    return tira;
}

void Escena::construirCasaYOfrenda() {
    Azar azar;
    Nodo* casa = raiz_->agregar("Casa");
    casa->colocar({0.0f, 0.0f, -16.0f});
    instanciar("casa_adobe", casa, "casa_adobe", {0, 0, 0});

    // La ofrenda se coloca sobre la banqueta del portal (0.15 m), frente al muro.
    Nodo* ofrenda = casa->agregar("Ofrenda");
    // Construir en coordenadas locales idénticas a la escena aprobada antes de colocar el conjunto.
    Nodo* aprobado=ofrenda->agregar("Altar aprobado");
    const int usosAntes=totalInstancias();
    construirAltarAprobado(aprobado);
    // Las nuevas envolventes texturizadas dejan espacio para 21 pétalos más
    // en el espolvoreado determinista, más las tres cubiertas de color: 3057 instancias.
    if(totalInstancias()-usosAntes!=3057)throw std::runtime_error("Cantidad de piezas importadas incorrecta");
    ofrenda->colocar({0.0f,0.15f,1.40f},{0,0,0},glm::vec3(.95f));
    // Ajuste autorizado: petate de 1.90 m de ancho, entre las bases de las columnas.
    if(auto* petate=aprobado->buscar("petate"))petate->escala.x=1.f/(1.5f*.95f);
    // Comprobar el conjunto ya ajustado contra las medidas reales del portal (marco local de la casa).
    aprobado->actualizar(ofrenda->matrizLocal());
    glm::vec3 minimo(1e20f),maximo(-1e20f);
    auto comprobarVolumen=[&](glm::vec3 lo,glm::vec3 hi,const glm::mat4& m) {
        glm::vec3 a(1e20f),b(-1e20f);
        for(int k=0;k<8;++k) {
            const auto v=glm::vec3(m*glm::vec4((k&1)?hi.x:lo.x,(k&2)?hi.y:lo.y,(k&4)?hi.z:lo.z,1));
            a=glm::min(a,v);b=glm::max(b,v);
        }
        minimo=glm::min(minimo,a);maximo=glm::max(maximo,b);
        if(a.z<.012f || b.z>3.30f || a.x< -3.60f || b.x>3.60f || a.y<.1499f || b.y>2.57f)
            throw std::runtime_error("La composición no cabe dentro del portal");
        for(float x:{-3.4f,-1.15f,1.15f,3.4f}) {
            if(b.x>x-.14f && a.x<x+.14f && b.z>2.86f && a.z<3.14f && b.y>.15f && a.y<.32f)
                throw std::runtime_error("Altar o petate intersecta una base de columna");
            if(b.x>x-.10f && a.x<x+.10f && b.z>2.90f && a.z<3.10f && b.y>.32f && a.y<2.62f)
                throw std::runtime_error("Altar intersecta una columna del portal");
        }
    };
    std::function<void(const Nodo*)> comprobarPortal=[&](const Nodo* n) {
        if(n->modelo && n->malla>=0)
            for(const auto& p:n->modelo->mallas[n->malla].primitivas)comprobarVolumen(p.min,p.max,n->mundo);
        if(n->lote)for(const auto& instancia:n->lote->transformaciones)
            for(const auto& parte:n->lote->modelo->plano)
                for(const auto& p:n->lote->modelo->mallas[parte.malla].primitivas)
                    comprobarVolumen(p.min,p.max,n->mundo*instancia*parte.matriz);
        for(const auto& hijo:n->hijos)comprobarPortal(hijo.get());
    };
    comprobarPortal(aprobado);
    registro::info("ENCAJE PORTAL: min=(%.4f,%.4f,%.4f), max=(%.4f,%.4f,%.4f); pared, suelo, techo y columnas libres",
        minimo.x,minimo.y,minimo.z,maximo.x,maximo.y,maximo.z);
    registro::info("ALTAR IMPORTADO: 3033 colocaciones; escala=0.95; casa Z=1.40; petate ancho=1.90 m");
    // El bloque anterior consumía 2 giros y 520×5 muestras para la cruz de pétalos.
    // Preservar la secuencia del papel del portal y las mariposas del entorno.
    for(int i=0;i<2602;++i)(void)azar(0,1);
    int k=0;

    // Papel picado colgado bajo el techo del portal
    for (float z : {1.75f, 2.75f})
        for (float x : {-1.6f, 1.6f}) {
            ++k;
            crearTiraPapel(casa, "papel_picado_portal_" + std::to_string(k),
                           {x, 2.45f, z}, azar(-4.0f, 4.0f), 3.0f, k - 1);
        }

    // Faroles a los lados de la entrada del portal
    instanciar("farol", casa, "farol_portal_izq", {-4.3f, 0.0f, 3.7f}, 0.0f);
    instanciar("farol", casa, "farol_portal_der", {4.3f, 0.0f, 3.7f}, 180.0f);
    instanciar("pumpkin", casa, "calabaza_portal_1", {-2.6f, 0.15f, 2.9f}, 15.0f);
    instanciar("pumpkin", casa, "calabaza_portal_2", {-2.25f, 0.15f, 3.05f}, 70.0f, 0.75f);
    instanciar("pumpkin-tall", casa, "calabaza_portal_3", {2.5f, 0.15f, 2.95f}, -30.0f, 0.9f);
    instanciar("maceta_cempasuchil", casa, "maceta_portal_1", {-3.2f, 0.15f, 2.4f}, 10.0f);
    instanciar("maceta_cempasuchil", casa, "maceta_portal_2", {3.2f, 0.15f, 2.4f}, -20.0f);

    // Mariposas monarca alrededor del arco
    k = 0;
    for (const glm::vec3& p : {glm::vec3(-1.1f, 2.5f, 1.6f), glm::vec3(0.8f, 2.2f, 2.3f), glm::vec3(1.7f, 1.6f, 1.2f)}) {
        Nodo* m = instanciar("mariposa_monarca", casa, "mariposa_portal_" + std::to_string(++k), p, azar(0, 360), 1.5f);
        m->rotacion.x = azar(-15.0f, 15.0f);
    }
}

// ---------------------------------------------------------------------------
// Camino de cempasúchil
// ---------------------------------------------------------------------------
void Escena::construirCamino() {
    Azar azar;
    Nodo* grupo = raiz_->agregar("Camino de cempasúchil");

    camino.construir({{0.0f, 0.0f, 19.0f},
                      {0.0f, 0.0f, 14.0f},
                      {-2.2f, 0.0f, 8.5f},
                      {1.6f, 0.0f, 2.5f},
                      {-1.3f, 0.0f, -3.5f},
                      {0.6f, 0.0f, -8.5f},
                      {0.0f, 0.0f, -12.5f}});
    const float L = camino.longitud();

    // Franja empedrada (malla procedural)
    {
        std::vector<Vertice> vs;
        std::vector<uint32_t> idx;
        camino.malla(2.7f, 1.6f, 0.008f, vs, idx);
        Material mat;
        mat.nombre = "empedrado";


        modelos_["camino_empedrado"] = Modelo::procedural("camino_empedrado", vs, idx, mat);
        catalogo_.push_back({"camino_empedrado", "Camino empedrado",
                             "Franja generada en código a partir de una spline Catmull-Rom centrípeta.", "Procedural",
                             modelos_["camino_empedrado"].get(), 1});
        Nodo* n = grupo->agregar("empedrado");
        n->modelo = modelos_["camino_empedrado"].get();
        n->malla = 0;
        n->categoria = "procedural";
    }

    // Alfombra de pétalos: franja con textura de recorte alfa (2 triángulos por tramo)
    {
        std::vector<Vertice> vs;
        std::vector<uint32_t> idx;
        const float ancho = 2.0f;
        camino.malla(ancho, ancho, 0.011f, vs, idx);
        Material mat;
        mat.nombre = "alfombra_petalos";


        modelos_["alfombra_petalos"] = Modelo::procedural("alfombra_petalos", vs, idx, mat);
        catalogo_.push_back({"alfombra_petalos", "Alfombra de pétalos",
                             "Franja procedural con textura de pétalos y recorte alfa; los pétalos 3D instanciados "
                             "agregan el relieve cercano.",
                             "Procedural", modelos_["alfombra_petalos"].get(), 1});
        Nodo* n = grupo->agregar("alfombra");
        n->modelo = modelos_["alfombra_petalos"].get();
        n->malla = 0;
        n->categoria = "procedural";
    }

    // Pétalos 3D: distribución normal alrededor del eje del camino (más densa al centro)
    Nodo* petalos = nodoLote("petalo_cempasuchil", grupo, "petalos");
    const int nPetalos = 7000;
    for (int i = 0; i < nPetalos; ++i) {
        float s = azar(0.3f, L - 0.2f);
        float lateral = std::clamp(azar.normal(0.36f), -1.05f, 1.05f);
        glm::vec3 p = camino.punto(s) + camino.izquierda(s) * lateral;
        p.y = 0.016f + azar(0.0f, 0.006f);
        glm::mat4 m = trs(p, azar(0, 360), azar(1.0f, 1.6f));
        m = m * glm::rotate(glm::mat4(1.0f), glm::radians(azar(-14.0f, 14.0f)), {1, 0, 0});
        glm::vec4 tinte(1.0f, azar(0.88f, 1.05f), azar(0.85f, 1.0f), 1.0f);
        if (azar(0, 1) < 0.05f) tinte = glm::vec4(0.95f, 0.30f, 0.45f, 1.0f);  // pétalos de mano de león
        petalos->lote->agregar(m, tinte);
    }
    info("petalo_cempasuchil")->usos += nPetalos;

    // Veladoras a ambos lados, cada 2.2 m
    Nodo* veladoras = nodoLote("veladora_1", grupo, "veladoras");
    int nVel = 0;
    for (float s = 1.2f; s < L - 0.8f; s += 2.2f)
        for (float lado : {-1.0f, 1.0f}) {
            glm::vec3 p = camino.punto(s + lado * 0.35f) + camino.izquierda(s) * (lado * 1.22f);
            p.y = 0.008f;
            veladoras->lote->agregar(trs(p, azar(0, 360)));
            ++nVel;
        }
    info("veladora_1")->usos += nVel;

    // Pares de faroles con una tira de papel picado entre ellos
    int k = 0;
    for (float s : {5.0f, 11.5f, 18.0f, 24.5f}) {
        ++k;
        glm::vec3 c = camino.punto(s);
        glm::vec3 izq = camino.izquierda(s);
        Nodo* par = grupo->agregar("faroles_" + std::to_string(k));
        par->colocar(c);
        // Orientar las caras de las linternas siguiendo el camino
        float haciaCasa = alinearX(camino.tangente(s));
        instanciar("farol", par, "farol_izq", izq * 1.55f, haciaCasa);
        instanciar("farol", par, "farol_der", -izq * 1.55f, haciaCasa);
        crearTiraPapel(par, "papel_picado", {0.0f, 2.52f, 0.0f}, alinearX(-izq), 3.1f, k + 1);
    }

    // Bancas para descansar junto al camino
    for (const auto& [s, lado] : {std::pair<float, float>{8.0f, 1.0f}, {20.5f, -1.0f}}) {
        glm::vec3 izq = camino.izquierda(s);
        glm::vec3 p = camino.punto(s) + izq * (lado * 2.7f);
        instanciar("bench", grupo, "banca_" + std::to_string(static_cast<int>(s)), p, alinearZ(-izq * lado), 1.8f);
    }

    // Mariposas monarca sobre el camino (almas que regresan)
    k = 0;
    for (float s : {3.0f, 7.5f, 9.0f, 13.0f, 16.5f, 21.0f, 23.0f, 27.5f, 30.0f}) {
        glm::vec3 p = camino.punto(s) + camino.izquierda(s) * azar(-1.3f, 1.3f);
        p.y = azar(1.0f, 2.4f);
        Nodo* m = instanciar("mariposa_monarca", grupo, "mariposa_" + std::to_string(++k), p, azar(0, 360), 1.5f);
        m->rotacion.x = azar(-20.0f, 20.0f);
        m->rotacion.z = azar(-15.0f, 15.0f);
    }

    // Personajes: una calaca camina hacia la ofrenda y un ánima la sigue
    {
        float s = 13.0f;
        Nodo* c = instanciar("character-skeleton", grupo, "calaca_caminante", camino.punto(s),
                             alinearZ(camino.tangente(s)), 2.0f);
        (void)c;
        float s2 = 9.0f;
        glm::vec3 p = camino.punto(s2) - camino.izquierda(s2) * 1.9f;
        p.y = 0.35f;
        instanciar("character-ghost", grupo, "anima", p, alinearZ(camino.tangente(s2)), 1.7f);
    }
}

// ---------------------------------------------------------------------------
// Panteón
// ---------------------------------------------------------------------------
void Escena::construirPanteon() {
    Azar azar;
    Nodo* panteon = raiz_->agregar("Panteón");
    instanciar("entrada_panteon", panteon, "portada", {0.0f, 0.0f, 16.0f}, 180.0f);
    instanciar("character-keeper", panteon, "panteonero", {3.4f, 0.0f, 14.6f}, 200.0f, 2.0f);

    // Pasillo central empedrado
    {
        Camino pasillo;
        pasillo.construir({{0.0f, 0.0f, 16.5f}, {0.0f, 0.0f, 31.5f}}, 4);
        std::vector<Vertice> vs;
        std::vector<uint32_t> idx;
        pasillo.malla(2.4f, 1.6f, 0.007f, vs, idx);
        Material mat = modelo("camino_empedrado")->materiales[0];
        modelos_["pasillo_panteon"] = Modelo::procedural("pasillo_panteon", vs, idx, mat);
        Nodo* n = panteon->agregar("pasillo");
        n->modelo = modelos_["pasillo_panteon"].get();
        n->malla = 0;
        n->categoria = "procedural";
    }

    // Reja frontal (12 tramos instanciados) y bardas de piedra (32 tramos)
    Nodo* reja = nodoLote("iron-fence", panteon, "reja_frontal");
    for (int i = 0; i < 6; ++i)
        for (float s : {-1.0f, 1.0f}) reja->lote->agregar(trs({s * (3.0f + 2.0f * i), 0.0f, 16.65f}, 0.0f, 2.0f));
    info("iron-fence")->usos += 12;

    Nodo* bardas = nodoLote("stone-wall", panteon, "bardas");
    for (int i = 0; i < 9; ++i) {
        float z = 17.0f + 2.0f * i;
        bardas->lote->agregar(trs({-13.2f, 0.0f, z}, 90.0f, 2.0f));
        bardas->lote->agregar(trs({13.2f, 0.0f, z}, -90.0f, 2.0f));
    }
    for (int i = 0; i < 14; ++i) bardas->lote->agregar(trs({-13.0f + 2.0f * i, 0.0f, 33.2f}, 180.0f, 2.0f));
    info("stone-wall")->usos += 32;
    Nodo* columnas = nodoLote("stone-wall-column", panteon, "columnas_barda");
    for (const glm::vec3& p : {glm::vec3(-14.0f, 0, 16.6f), glm::vec3(14.0f, 0, 16.6f), glm::vec3(-14.0f, 0, 34.0f),
                               glm::vec3(14.0f, 0, 34.0f)})
        columnas->lote->agregar(trs(p + glm::vec3(0, 0, 0.8f), 0.0f, 2.0f));
    info("stone-wall-column")->usos += 4;

    // Tumbas en tres filas a cada lado del pasillo, adornadas con flores y veladoras
    Nodo* tumbas = panteon->agregar("Tumbas");
    Nodo* flores = nodoLote("flor_cempasuchil", tumbas, "flores_tumbas");
    Nodo* velas = nodoLote("veladora_1", tumbas, "veladoras_tumbas");
    const char* lapidas[] = {"gravestone-cross", "gravestone-round", "gravestone-bevel", "gravestone-decorative",
                             "cross-wood"};
    int k = 0, nFlores = 0, nVelas = 0;
    for (float z : {19.8f, 23.6f, 27.4f}) {
        for (float x : {-11.0f, -7.5f, -4.0f, 4.0f, 7.5f, 11.0f}) {
            ++k;
            Nodo* t = tumbas->agregar("tumba_" + std::to_string(k));
            t->colocar({x + azar(-0.2f, 0.2f), 0.0f, z + azar(-0.2f, 0.2f)}, {0.0f, azar(-4.0f, 4.0f), 0.0f});
            Nodo* monticulo = instanciar("grave", t, "monticulo", {0, 0, 0.05f}, 180.0f, 2.0f);
            monticulo->tinte = glm::vec4(0.62f, 0.46f, 0.38f, 1.0f);  // tierra recién removida
            Nodo* bordillo = instanciar("grave-border", t, "bordillo", {0, 0, 0}, 180.0f, 2.0f);
            bordillo->tinte = glm::vec4(0.80f, 0.74f, 0.70f, 1.0f);   // piedra de cantera
            const char* lap = lapidas[(k * 7) % 5];
            instanciar(lap, t, "lapida", {0.0f, 0.0f, 1.55f}, 180.0f, std::string(lap) == "cross-wood" ? 1.7f : 1.6f);
            // flores sembradas sobre el montículo
            for (int f = 0; f < 9; ++f) {
                glm::vec3 p(azar(-0.40f, 0.40f), 0.22f, azar(-0.9f, 0.85f));
                flores->lote->agregar(t->matrizLocal() * trs(p, azar(0, 360), azar(1.1f, 1.5f)));
                ++nFlores;
            }
            for (float s : {-1.0f, 1.0f}) {
                velas->lote->agregar(t->matrizLocal() * trs({s * 0.95f, 0.0f, -1.35f}, azar(0, 360)));
                ++nVelas;
            }
            if (k % 4 == 1) instanciar("candle-multiple", t, "velas", {0.55f, 0.0f, 1.25f}, azar(0, 360), 1.2f);
            if (k % 5 == 2) instanciar("urn-round", t, "urna", {-0.55f, 0.0f, 1.3f}, 0.0f, 1.4f);
            if (k % 6 == 3) instanciar("calaverita_azucar", t, "calaverita", {0.0f, 0.22f, -0.95f}, 0.0f, 1.2f);
            if (k % 6 == 0) instanciar("pan_de_muerto", t, "pan", {0.25f, 0.2f, -0.6f}, azar(0, 90));
        }
    }
    info("flor_cempasuchil")->usos += nFlores;
    info("veladora_1")->usos += nVelas;

    // Criptas y cruz atrial al fondo
    instanciar("crypt", panteon, "cripta_izq", {-9.0f, 0.0f, 31.0f}, 180.0f, 3.0f);
    instanciar("crypt", panteon, "cripta_der", {9.0f, 0.0f, 31.0f}, 180.0f, 3.0f);
    instanciar("cross-column", panteon, "cruz_atrial", {0.0f, 0.0f, 32.2f}, 180.0f, 2.2f);

    // Cipreses dentro y detrás del panteón
    Nodo* cipreses = nodoLote("pine", panteon, "cipreses");
    for (float z : {18.5f, 25.0f, 30.0f})
        for (float x : {-12.4f, 12.4f})
            cipreses->lote->agregar(trs({x, 0.0f, z}, azar(0, 360), azar(2.5f, 3.0f)), glm::vec4(0.62f, 0.76f, 0.64f, 1.0f));
    for (float x : {-10.0f, -4.0f, 3.0f, 9.5f})
        cipreses->lote->agregar(trs({x, 0.0f, 36.0f}, azar(0, 360), azar(2.6f, 3.2f)), glm::vec4(0.62f, 0.76f, 0.64f, 1.0f));
    info("pine")->usos += cipreses->lote->instancias();
}

// ---------------------------------------------------------------------------
// Alrededores: árboles, rocas y casas del pueblo
// ---------------------------------------------------------------------------
void Escena::construirAlrededores() {
    Azar azar;
    Nodo* grupo = raiz_->agregar("Alrededores");
    Nodo* arboles = nodoLote("pine-crooked", grupo, "arboles");
    for (const glm::vec3& p : {glm::vec3(-7.0f, 0, 10.0f), glm::vec3(6.5f, 0, 6.0f), glm::vec3(-6.5f, 0, -1.0f),
                               glm::vec3(7.0f, 0, -7.0f), glm::vec3(-7.5f, 0, -10.0f), glm::vec3(8.0f, 0, 12.0f),
                               glm::vec3(-9.5f, 0, 3.0f), glm::vec3(10.5f, 0, 0.5f), glm::vec3(-17.0f, 0, 14.0f),
                               glm::vec3(18.0f, 0, 10.0f), glm::vec3(-12.0f, 0, -20.0f), glm::vec3(12.0f, 0, -21.0f)})
        arboles->lote->agregar(trs(p, azar(0, 360), azar(2.4f, 3.2f)), glm::vec4(0.72f, 0.82f, 0.70f, 1.0f));
    info("pine-crooked")->usos += arboles->lote->instancias();

    Nodo* rocas = nodoLote("rocks", grupo, "rocas");
    for (int i = 0; i < 14; ++i) {
        float a = azar(0, 6.2831f), r = azar(5.0f, 22.0f);
        glm::vec3 p(std::cos(a) * r, 0.0f, std::sin(a) * r * 1.2f);
        if (std::abs(p.x) < 3.5f) p.x += p.x < 0 ? -3.5f : 3.5f;
        if (p.z > 15.0f && p.z < 35.0f && std::abs(p.x) < 15.0f) p.x = p.x < 0 ? -16.5f : 16.5f;
        float g = azar(0.55f, 0.75f);
        rocas->lote->agregar(trs(p, azar(0, 360), azar(1.0f, 2.0f)), glm::vec4(g, g * 0.95f, g * 0.9f, 1.0f));
    }
    info("rocks")->usos += rocas->lote->instancias();

    // Casas vecinas (instancias del mismo modelo, sin costo extra de memoria)
    instanciar("casa_adobe", grupo, "casa_vecina_1", {-14.5f, 0.0f, -6.0f}, 75.0f);
    instanciar("casa_adobe", grupo, "casa_vecina_2", {15.5f, 0.0f, -3.0f}, -80.0f);
}
