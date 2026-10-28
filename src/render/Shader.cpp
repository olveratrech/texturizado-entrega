#include "render/Shader.h"

#include "Registro.h"

#include <fstream>
#include <sstream>
#include <vector>

static bool leerArchivo(const std::string& ruta, std::string& salida) {
    std::ifstream f(ruta, std::ios::binary);
    if (!f) return false;
    std::stringstream ss;
    ss << f.rdbuf();
    salida = ss.str();
    return true;
}

static std::string insertarDefiniciones(const std::string& fuente, const std::string& definiciones) {
    if (definiciones.empty()) return fuente;
    size_t pos = fuente.find("#version");
    if (pos == std::string::npos) return definiciones + fuente;
    size_t fin = fuente.find('\n', pos);
    return fuente.substr(0, fin + 1) + definiciones + fuente.substr(fin + 1);
}

static GLuint compilar(GLenum tipo, const std::string& fuente, const std::string& ruta) {
    GLuint s = glCreateShader(tipo);
    const char* c = fuente.c_str();
    glShaderSource(s, 1, &c, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint largo = 0;
        glGetShaderiv(s, GL_INFO_LOG_LENGTH, &largo);
        std::vector<char> log(static_cast<size_t>(largo) + 1);
        glGetShaderInfoLog(s, largo, nullptr, log.data());
        registro::error("Error al compilar %s:\n%s", ruta.c_str(), log.data());
        glDeleteShader(s);
        return 0;
    }
    return s;
}

bool Shader::cargar(const std::string& rutaVert, const std::string& rutaFrag, const std::string& definiciones) {
    std::string fv, ff;
    if (!leerArchivo(rutaVert, fv) || !leerArchivo(rutaFrag, ff)) {
        registro::error("No se encontró el shader %s / %s", rutaVert.c_str(), rutaFrag.c_str());
        return false;
    }
    GLuint v = compilar(GL_VERTEX_SHADER, insertarDefiniciones(fv, definiciones), rutaVert);
    GLuint f = compilar(GL_FRAGMENT_SHADER, insertarDefiniciones(ff, definiciones), rutaFrag);
    if (!v || !f) return false;

    id_ = glCreateProgram();
    glAttachShader(id_, v);
    glAttachShader(id_, f);
    glLinkProgram(id_);
    glDeleteShader(v);
    glDeleteShader(f);

    GLint ok = 0;
    glGetProgramiv(id_, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint largo = 0;
        glGetProgramiv(id_, GL_INFO_LOG_LENGTH, &largo);
        std::vector<char> log(static_cast<size_t>(largo) + 1);
        glGetProgramInfoLog(id_, largo, nullptr, log.data());
        registro::error("Error al enlazar %s + %s:\n%s", rutaVert.c_str(), rutaFrag.c_str(), log.data());
        glDeleteProgram(id_);
        id_ = 0;
        return false;
    }
    cache_.clear();
    return true;
}

void Shader::liberar() {
    if (id_) glDeleteProgram(id_);
    id_ = 0;
    cache_.clear();
}

GLint Shader::ubicacion(const char* nombre) const {
    auto it = cache_.find(nombre);
    if (it != cache_.end()) return it->second;
    GLint loc = glGetUniformLocation(id_, nombre);
    cache_.emplace(nombre, loc);
    return loc;
}
