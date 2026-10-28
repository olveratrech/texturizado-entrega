#include "scene/Camino.h"

#include <algorithm>
#include <cmath>

// Catmull-Rom centrípeta (alfa = 0.5) evaluada con el algoritmo de Barry-Goldman.
static glm::vec3 catmullRom(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3,
                            float u) {
    auto nudo = [](float t, const glm::vec3& a, const glm::vec3& b) {
        return t + std::max(std::sqrt(glm::length(b - a)), 1e-4f);
    };
    float t0 = 0.0f, t1 = nudo(t0, p0, p1), t2 = nudo(t1, p1, p2), t3 = nudo(t2, p2, p3);
    float t = t1 + (t2 - t1) * u;
    glm::vec3 a1 = (t1 - t) / (t1 - t0) * p0 + (t - t0) / (t1 - t0) * p1;
    glm::vec3 a2 = (t2 - t) / (t2 - t1) * p1 + (t - t1) / (t2 - t1) * p2;
    glm::vec3 a3 = (t3 - t) / (t3 - t2) * p2 + (t - t2) / (t3 - t2) * p3;
    glm::vec3 b1 = (t2 - t) / (t2 - t0) * a1 + (t - t0) / (t2 - t0) * a2;
    glm::vec3 b2 = (t3 - t) / (t3 - t1) * a2 + (t - t1) / (t3 - t1) * a3;
    return (t2 - t) / (t2 - t1) * b1 + (t - t1) / (t2 - t1) * b2;
}

void Camino::construir(const std::vector<glm::vec3>& control, int muestrasPorTramo) {
    control_ = control;
    muestras_.clear();
    s_.clear();
    if (control.size() < 2) return;
    // puntos fantasma en los extremos (reflexión) para que la curva llegue a ellos
    std::vector<glm::vec3> p;
    p.push_back(2.0f * control[0] - control[1]);
    p.insert(p.end(), control.begin(), control.end());
    p.push_back(2.0f * control.back() - control[control.size() - 2]);
    for (size_t i = 1; i + 2 < p.size(); ++i) {
        for (int k = 0; k < muestrasPorTramo; ++k)
            muestras_.push_back(catmullRom(p[i - 1], p[i], p[i + 1], p[i + 2], static_cast<float>(k) / muestrasPorTramo));
    }
    muestras_.push_back(control.back());
    s_.resize(muestras_.size());
    s_[0] = 0.0f;
    for (size_t i = 1; i < muestras_.size(); ++i) s_[i] = s_[i - 1] + glm::length(muestras_[i] - muestras_[i - 1]);
}

glm::vec3 Camino::punto(float s) const {
    if (muestras_.empty()) return glm::vec3(0.0f);
    s = std::clamp(s, 0.0f, longitud());
    size_t i = static_cast<size_t>(std::upper_bound(s_.begin(), s_.end(), s) - s_.begin());
    if (i == 0) return muestras_.front();
    if (i >= muestras_.size()) return muestras_.back();
    float tramo = s_[i] - s_[i - 1];
    float f = tramo > 1e-6f ? (s - s_[i - 1]) / tramo : 0.0f;
    return glm::mix(muestras_[i - 1], muestras_[i], f);
}

glm::vec3 Camino::tangente(float s) const {
    const float h = 0.05f;
    glm::vec3 d = punto(s + h) - punto(s - h);
    d.y = 0.0f;
    float l = glm::length(d);
    return l > 1e-6f ? d / l : glm::vec3(0.0f, 0.0f, -1.0f);
}

glm::vec3 Camino::izquierda(float s) const {
    glm::vec3 t = tangente(s);
    return glm::vec3(t.z, 0.0f, -t.x);  // arriba x tangente
}

void Camino::malla(float ancho, float escalaUV, float altura, std::vector<Vertice>& vs,
                   std::vector<uint32_t>& idx) const {
    vs.clear();
    idx.clear();
    const float paso = 0.25f;
    int n = std::max(2, static_cast<int>(longitud() / paso) + 1);
    for (int i = 0; i < n; ++i) {
        float s = longitud() * static_cast<float>(i) / static_cast<float>(n - 1);
        glm::vec3 c = punto(s);
        glm::vec3 l = izquierda(s);
        // los extremos se angostan para que la franja no termine en un corte recto
        float k = std::min({1.0f, s / 1.5f + 0.35f, (longitud() - s) / 1.5f + 0.35f});
        float w = ancho * 0.5f * k;
        glm::vec3 pl = c + l * w, pr = c - l * w;
        pl.y = pr.y = altura;
        vs.push_back({pl, glm::vec3(0, 1, 0), glm::vec2(0.0f, s / escalaUV)});
        vs.push_back({pr, glm::vec3(0, 1, 0), glm::vec2(2.0f * w / escalaUV, s / escalaUV)});
        if (i > 0) {
            uint32_t b = static_cast<uint32_t>(2 * (i - 1));
            idx.insert(idx.end(), {b, b + 1, b + 2, b + 1, b + 3, b + 2});
        }
    }
}
