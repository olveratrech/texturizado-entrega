#include "render/Model.h"

#include "Registro.h"
#include "render/ImageLoader.h"

#include <cgltf.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <utility>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Lectura de archivos para cgltf con soporte de rutas UTF-8 en Windows
// ---------------------------------------------------------------------------
static cgltf_result leerArchivo(const cgltf_memory_options*, const cgltf_file_options*, const char* ruta,
                                cgltf_size* tam, void** datos) {
#ifdef _WIN32
    FILE* f = _wfopen(fs::u8path(ruta).wstring().c_str(), L"rb");
#else
    FILE* f = std::fopen(ruta, "rb");
#endif
    if (!f) return cgltf_result_file_not_found;
    std::fseek(f, 0, SEEK_END);
    long n = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    void* buf = std::malloc(static_cast<size_t>(n));
    if (!buf) {
        std::fclose(f);
        return cgltf_result_out_of_memory;
    }
    size_t leidos = std::fread(buf, 1, static_cast<size_t>(n), f);
    std::fclose(f);
    if (leidos != static_cast<size_t>(n)) {
        std::free(buf);
        return cgltf_result_io_error;
    }
    *tam = static_cast<cgltf_size>(n);
    *datos = buf;
    return cgltf_result_success;
}

static void liberarArchivo(const cgltf_memory_options*, const cgltf_file_options*, void* datos) {
    std::free(datos);
}

static std::string decodificarUri(const char* uri) {
    if (!uri) return {};
    std::vector<char> texto(uri, uri + std::strlen(uri) + 1);
    cgltf_decode_uri(texto.data());
    return texto.data();
}

static bool bytesImagen(const cgltf_image& imagen, const fs::path& carpeta,
                        std::vector<std::uint8_t>& salida) {
    if (imagen.buffer_view && imagen.buffer_view->buffer && imagen.buffer_view->buffer->data) {
        const auto* inicio = static_cast<const std::uint8_t*>(imagen.buffer_view->buffer->data) +
                             imagen.buffer_view->offset;
        salida.assign(inicio, inicio + imagen.buffer_view->size);
        return true;
    }
    if (!imagen.uri || std::strncmp(imagen.uri, "data:", 5) == 0) return false;
    const fs::path ruta = carpeta / fs::u8path(decodificarUri(imagen.uri));
    std::ifstream archivo(ruta, std::ios::binary | std::ios::ate);
    if (!archivo) return false;
    const std::streamsize tam = archivo.tellg();
    if (tam <= 0) return false;
    salida.resize(static_cast<std::size_t>(tam));
    archivo.seekg(0);
    return static_cast<bool>(archivo.read(reinterpret_cast<char*>(salida.data()), tam));
}

static GLenum envoltura(cgltf_wrap_mode modo) {
    if (modo == cgltf_wrap_mode_clamp_to_edge) return GL_CLAMP_TO_EDGE;
    if (modo == cgltf_wrap_mode_mirrored_repeat) return GL_MIRRORED_REPEAT;
    return GL_REPEAT;
}

static bool filtroCercano(cgltf_filter_type filtro) {
    return filtro == cgltf_filter_type_nearest || filtro == cgltf_filter_type_nearest_mipmap_nearest ||
           filtro == cgltf_filter_type_nearest_mipmap_linear;
}

static GLuint subirTextura(const cgltf_texture& textura, const fs::path& carpeta, bool srgb) {
    const cgltf_image* imagen = textura.image;
    if (!imagen) return 0;
    std::vector<std::uint8_t> bytes;
    ImagenRGBA rgba;
    if (!bytesImagen(*imagen, carpeta, bytes) || !decodificarImagen(bytes.data(), bytes.size(), rgba)) {
        registro::error("No se pudo decodificar la textura %s", imagen->uri ? imagen->uri : "(embebida)");
        return 0;
    }

    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8, rgba.ancho, rgba.alto, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, rgba.pixeles.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    const cgltf_sampler* s = textura.sampler;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, s ? envoltura(s->wrap_s) : GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, s ? envoltura(s->wrap_t) : GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    s && filtroCercano(s->mag_filter) ? GL_NEAREST : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    s && filtroCercano(s->min_filter) ? GL_NEAREST_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    return id;
}

// ---------------------------------------------------------------------------
// GPU
// ---------------------------------------------------------------------------
void configurarAtributosVertice() {
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertice), reinterpret_cast<void*>(offsetof(Vertice, pos)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertice), reinterpret_cast<void*>(offsetof(Vertice, normal)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertice), reinterpret_cast<void*>(offsetof(Vertice, uv)));
}

Primitiva crearPrimitiva(const std::vector<Vertice>& vertices, const std::vector<uint32_t>& indices, int material) {
    Primitiva p;
    p.material = material;
    p.vertices = static_cast<int>(vertices.size());
    p.indices = static_cast<GLsizei>(indices.size());
    p.min = glm::vec3(1e30f);
    p.max = glm::vec3(-1e30f);

    for (const auto& v : vertices) {
        p.min = glm::min(p.min, v.pos);
        p.max = glm::max(p.max, v.pos);

    }


    if(Modelo::soloCPU)return p;
    glGenVertexArrays(1, &p.vao);
    glGenBuffers(1, &p.vbo);
    glGenBuffers(1, &p.ebo);
    glBindVertexArray(p.vao);
    glBindBuffer(GL_ARRAY_BUFFER, p.vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertice)), vertices.data(),
                 GL_STATIC_DRAW);
    configurarAtributosVertice();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, p.ebo);
    // Optimización: índices de 16 bits (la mitad de memoria) cuando alcanzan
    if (vertices.size() < 65536) {
        std::vector<uint16_t> i16(indices.begin(), indices.end());
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(i16.size() * sizeof(uint16_t)), i16.data(),
                     GL_STATIC_DRAW);
        p.tipoIndice = GL_UNSIGNED_SHORT;
    } else {
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(uint32_t)),
                     indices.data(), GL_STATIC_DRAW);
        p.tipoIndice = GL_UNSIGNED_INT;
    }
    glBindVertexArray(0);
    return p;
}

// ---------------------------------------------------------------------------
// Carga glTF
// ---------------------------------------------------------------------------
bool Modelo::cargar(const std::string& rutaArchivo) {
    ruta = rutaArchivo;
    nombre = fs::u8path(rutaArchivo).stem().u8string();


    cgltf_options opciones{};
    opciones.file.read = leerArchivo;
    opciones.file.release = liberarArchivo;
    cgltf_data* d = nullptr;
    if (cgltf_parse_file(&opciones, rutaArchivo.c_str(), &d) != cgltf_result_success) {
        registro::error("No se pudo leer el modelo %s", rutaArchivo.c_str());
        return false;
    }
    if (cgltf_load_buffers(&opciones, d, rutaArchivo.c_str()) != cgltf_result_success) {
        registro::error("No se pudieron cargar los buffers de %s", rutaArchivo.c_str());
        cgltf_free(d);
        return false;
    }

    // --- materiales y texturas ------------------------------------------------
    const fs::path carpetaModelo = fs::u8path(rutaArchivo).parent_path();
    std::map<std::pair<const cgltf_texture*, bool>, GLuint> cacheTexturas;
    auto textura = [&](const cgltf_texture_view& vista, bool srgb) {
        if (soloCPU || !vista.texture) return GLuint(0);
        const auto clave = std::make_pair(static_cast<const cgltf_texture*>(vista.texture), srgb);
        auto encontrada = cacheTexturas.find(clave);
        if (encontrada != cacheTexturas.end()) return encontrada->second;
        const GLuint id = subirTextura(*vista.texture, carpetaModelo, srgb);
        cacheTexturas.emplace(clave, id);
        if (id) texturas.push_back(id);
        return id;
    };
    for (cgltf_size i = 0; i < d->materials_count; ++i) {
        const cgltf_material& m = d->materials[i];
        Material mat;
        if (m.name) mat.nombre = m.name;
        if (m.has_pbr_metallic_roughness) {
            const auto& pbr = m.pbr_metallic_roughness;
            mat.colorBase = glm::make_vec4(pbr.base_color_factor);
            mat.metalico = pbr.metallic_factor;
            mat.rugosidad = pbr.roughness_factor;
            mat.texturaColor = textura(pbr.base_color_texture, true);
            mat.texturaMetalRugosidad = textura(pbr.metallic_roughness_texture, false);
            if (pbr.base_color_texture.texture) mat.texcoord = pbr.base_color_texture.texcoord;
            else if (pbr.metallic_roughness_texture.texture) mat.texcoord = pbr.metallic_roughness_texture.texcoord;
        }
        mat.emision = glm::make_vec3(m.emissive_factor) *
                      (m.has_emissive_strength ? m.emissive_strength.emissive_strength : 1.0f);
        mat.texturaNormal = textura(m.normal_texture, false);
        mat.texturaOclusion = textura(m.occlusion_texture, false);
        mat.texturaEmision = textura(m.emissive_texture, true);
        mat.escalaNormal = m.normal_texture.texture ? m.normal_texture.scale : 1.0f;
        mat.oclusion = m.occlusion_texture.texture ? m.occlusion_texture.scale : 1.0f;
        mat.especular = m.has_specular ? m.specular.specular_factor : 1.0f;
        mat.transmision = m.has_transmission ? m.transmission.transmission_factor : 0.0f;
        mat.ior = m.has_ior ? m.ior.ior : 1.5f;
        std::string nombreMaterial = mat.nombre;
        std::transform(nombreMaterial.begin(), nombreMaterial.end(), nombreMaterial.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (mat.transmision > 0.0f) {
            mat.tipoTransparente = 1;
            if (nombreMaterial.find("agua") != std::string::npos ||
                nombreMaterial.find("cerveza") != std::string::npos)
                mat.tipoTransparente = 2;
        }
        // Ajustes artísticos de los líquidos solicitados para esta escena. Se aplican
        // sobre el factor exportado, sin alterar las texturas ni el vidrio envolvente.
        if (nombreMaterial.find("victoria cerveza") != std::string::npos)
            mat.colorBase *= glm::vec4(0.42f, 0.25f, 0.12f, 1.0f);
        if (mat.tipoTransparente == 2 && nombreMaterial.find("agua") != std::string::npos)
            mat.colorBase *= glm::vec4(0.12f, 0.42f, 1.0f, 1.0f);
        if (!m.has_pbr_metallic_roughness && m.normal_texture.texture) mat.texcoord = m.normal_texture.texcoord;
        mat.mezcla = m.alpha_mode == cgltf_alpha_mode_blend || mat.transmision > 0.0f;
        mat.mascara = m.alpha_mode == cgltf_alpha_mode_mask;
        mat.corteAlfa = m.alpha_cutoff > 0.0f ? m.alpha_cutoff : 0.5f;
        mat.sinLuz = m.unlit;
        mat.dobleCara = m.double_sided;
        materiales.push_back(mat);
    }
    if (materiales.empty()) materiales.emplace_back();

    // --- mallas -----------------------------------------------------------------
    for (cgltf_size i = 0; i < d->meshes_count; ++i) {
        const cgltf_mesh& cm = d->meshes[i];
        Malla malla;
        if (cm.name) malla.nombre = cm.name;
        for (cgltf_size j = 0; j < cm.primitives_count; ++j) {
            const cgltf_primitive& pr = cm.primitives[j];
            if (pr.type != cgltf_primitive_type_triangles) continue;
            int mat = pr.material ? static_cast<int>(cgltf_material_index(d, pr.material)) : 0;
            if (mat < 0 || static_cast<size_t>(mat) >= materiales.size()) mat = 0;
            const int uvMaterial = materiales[static_cast<size_t>(mat)].texcoord;
            const cgltf_accessor *apos = nullptr, *anor = nullptr, *auv = nullptr;
            for (cgltf_size k = 0; k < pr.attributes_count; ++k) {
                const cgltf_attribute& a = pr.attributes[k];
                if (a.type == cgltf_attribute_type_position) apos = a.data;
                else if (a.type == cgltf_attribute_type_normal) anor = a.data;
                else if (a.type == cgltf_attribute_type_texcoord && a.index == uvMaterial) auv = a.data;
            }
            if (!apos) continue;
            std::vector<Vertice> vs(apos->count);
            for (cgltf_size k = 0; k < apos->count; ++k) {
                cgltf_accessor_read_float(apos, k, &vs[k].pos.x, 3);
                vs[k].normal = glm::vec3(0.0f, 1.0f, 0.0f);
                vs[k].uv = glm::vec2(0.0f);
                if (anor) cgltf_accessor_read_float(anor, k, &vs[k].normal.x, 3);
                if (auv) cgltf_accessor_read_float(auv, k, &vs[k].uv.x, 2);
            }
            std::vector<uint32_t> idx;
            if (pr.indices) {
                idx.resize(pr.indices->count);
                for (cgltf_size k = 0; k < pr.indices->count; ++k)
                    idx[k] = static_cast<uint32_t>(cgltf_accessor_read_index(pr.indices, k));
            } else {
                idx.resize(vs.size());
                for (size_t k = 0; k < idx.size(); ++k) idx[k] = static_cast<uint32_t>(k);
            }
            if (!anor) {  // normales planas si el archivo no las trae
                for (size_t k = 0; k + 2 < idx.size(); k += 3) {
                    glm::vec3 n = glm::normalize(glm::cross(vs[idx[k + 1]].pos - vs[idx[k]].pos,
                                                            vs[idx[k + 2]].pos - vs[idx[k]].pos));
                    vs[idx[k]].normal = vs[idx[k + 1]].normal = vs[idx[k + 2]].normal = n;
                }
            }
            if(nombre=="pan_de_muerto"||nombre.rfind("dulce_",0)==0)
                for(auto v:idx)malla.superficie.push_back(vs[v].pos);
            malla.primitivas.push_back(crearPrimitiva(vs, idx, mat));
            triangulos += static_cast<int>(idx.size() / 3);
            vertices += static_cast<int>(vs.size());
        }
        mallas.push_back(std::move(malla));
    }

    // --- nodos y jerarquía ------------------------------------------------------
    nodos.resize(d->nodes_count);
    for (cgltf_size i = 0; i < d->nodes_count; ++i) {
        const cgltf_node& cn = d->nodes[i];
        NodoModelo& n = nodos[i];
        n.nombre = cn.name ? cn.name : ("nodo_" + std::to_string(i));
        if (cn.has_matrix) {
            glm::mat4 m = glm::make_mat4(cn.matrix);
            glm::vec3 sesgo;
            glm::vec4 perspectiva;
            glm::decompose(m, n.escala, n.rotacion, n.traslacion, sesgo, perspectiva);
        } else {
            if (cn.has_translation) n.traslacion = glm::make_vec3(cn.translation);
            if (cn.has_rotation) n.rotacion = glm::quat(cn.rotation[3], cn.rotation[0], cn.rotation[1], cn.rotation[2]);
            if (cn.has_scale) n.escala = glm::make_vec3(cn.scale);
        }
        n.malla = cn.mesh ? static_cast<int>(cgltf_mesh_index(d, cn.mesh)) : -1;
        for (cgltf_size k = 0; k < cn.children_count; ++k)
            n.hijos.push_back(static_cast<int>(cgltf_node_index(d, cn.children[k])));
    }
    const cgltf_scene* escena = d->scene ? d->scene : (d->scenes_count ? &d->scenes[0] : nullptr);
    if (escena) {
        for (cgltf_size i = 0; i < escena->nodes_count; ++i)
            raices.push_back(static_cast<int>(cgltf_node_index(d, escena->nodes[i])));
    } else {
        for (cgltf_size i = 0; i < d->nodes_count; ++i)
            if (!d->nodes[i].parent) raices.push_back(static_cast<int>(i));
    }
    // La esquina inferior de una caja rotada puede estar por debajo de la malla.
    // Usar vértices reales evita trasladar un modelo demasiado arriba del mantel.
    baseY = 1e30f;
    std::vector<const cgltf_node*> pendientes;
    for(int r:raices) pendientes.push_back(&d->nodes[r]);
    while(!pendientes.empty()) {
        const cgltf_node* n=pendientes.back();pendientes.pop_back();
        cgltf_float datosMatriz[16];cgltf_node_transform_world(n,datosMatriz);
        glm::mat4 m=glm::make_mat4(datosMatriz);
        if(n->mesh) for(cgltf_size j=0;j<n->mesh->primitives_count;++j) {
            const auto& pr=n->mesh->primitives[j];
            if(pr.type!=cgltf_primitive_type_triangles)continue;
            for(cgltf_size a=0;a<pr.attributes_count;++a) {
                if(pr.attributes[a].type!=cgltf_attribute_type_position)continue;
                const auto* pos=pr.attributes[a].data;
                cgltf_size cantidad=pr.indices?pr.indices->count:pos->count;
                for(cgltf_size v=0;v<cantidad;++v) {
                    cgltf_size indice=pr.indices?cgltf_accessor_read_index(pr.indices,v):v;
                    glm::vec3 punto;cgltf_accessor_read_float(pos,indice,&punto.x,3);
                    baseY=std::min(baseY,(m*glm::vec4(punto,1)).y);
                }
            }
        }
        for(cgltf_size j=0;j<n->children_count;++j)pendientes.push_back(n->children[j]);
    }
    cgltf_free(d);
    calcularPlano();
    if(baseY>1e29f)baseY=min.y;
    return true;
}

glm::mat4 Modelo::matrizNodo(const NodoModelo& n) {
    return glm::translate(glm::mat4(1.0f), n.traslacion) * glm::mat4_cast(n.rotacion) *
           glm::scale(glm::mat4(1.0f), n.escala);
}

void Modelo::calcularPlano() {
    plano.clear();
    min = glm::vec3(1e30f);
    max = glm::vec3(-1e30f);
    struct Pila {
        int nodo;
        glm::mat4 m;
    };
    std::vector<Pila> pila;
    for (int r : raices) pila.push_back({r, glm::mat4(1.0f)});
    while (!pila.empty()) {
        Pila p = pila.back();
        pila.pop_back();
        const NodoModelo& n = nodos[static_cast<size_t>(p.nodo)];
        glm::mat4 m = p.m * matrizNodo(n);
        if (n.malla >= 0) {
            plano.push_back({n.malla, m});
            for (const auto& pr : mallas[static_cast<size_t>(n.malla)].primitivas) {
                for (int k = 0; k < 8; ++k) {
                    glm::vec3 c((k & 1) ? pr.max.x : pr.min.x, (k & 2) ? pr.max.y : pr.min.y,
                                (k & 4) ? pr.max.z : pr.min.z);
                    glm::vec3 w = glm::vec3(m * glm::vec4(c, 1.0f));
                    min = glm::min(min, w);
                    max = glm::max(max, w);
                }
            }
        }
        for (int h : n.hijos) pila.push_back({h, m});
    }
    if (plano.empty()) min = max = glm::vec3(0.0f);
}

std::unique_ptr<Modelo> Modelo::procedural(const std::string& nombre, const std::vector<Vertice>& vertices,
                                           const std::vector<uint32_t>& indices, const Material& material) {
    auto m = std::make_unique<Modelo>();
    m->nombre = nombre;
    m->ruta = "(procedural)";
    m->materiales.push_back(material);
    Malla malla;
    malla.nombre = nombre;
    malla.primitivas.push_back(crearPrimitiva(vertices, indices, 0));
    m->mallas.push_back(std::move(malla));
    NodoModelo n;
    n.nombre = nombre;
    n.malla = 0;
    m->nodos.push_back(n);
    m->raices.push_back(0);
    m->triangulos = static_cast<int>(indices.size() / 3);
    m->vertices = static_cast<int>(vertices.size());
    m->calcularPlano();
    return m;
}

void Modelo::liberar() {
    for (auto& malla : mallas) {
        for (auto& p : malla.primitivas) {
            if (p.vao) glDeleteVertexArrays(1, &p.vao);
            if (p.vbo) glDeleteBuffers(1, &p.vbo);
            if (p.ebo) glDeleteBuffers(1, &p.ebo);
            p.vao = p.vbo = p.ebo = 0;
        }
    }
    for (GLuint textura : texturas)
        if (textura) glDeleteTextures(1, &textura);
    texturas.clear();
    mallas.clear();
}
