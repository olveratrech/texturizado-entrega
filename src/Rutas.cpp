#include "Rutas.h"

#include <cstdlib>
#include <filesystem>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace rutas {

static std::string g_base = ".";

static fs::path carpetaEjecutable(const char* argv0) {
#ifdef _WIN32
    wchar_t buffer[MAX_PATH];
    DWORD n = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (n > 0) return fs::path(std::wstring(buffer, n)).parent_path();
#elif defined(__linux__)
    char buffer[4096];
    ssize_t n = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (n > 0) return fs::path(std::string(buffer, static_cast<size_t>(n))).parent_path();
#endif
    std::error_code ec;
    return fs::absolute(fs::path(argv0 ? argv0 : "."), ec).parent_path();
}

bool inicializar(const char* argv0) {
    std::error_code ec;
    std::vector<fs::path> candidatas;
    for (const fs::path& inicio : {carpetaEjecutable(argv0), fs::current_path(ec)}) {
        fs::path p = inicio;
        for (int nivel = 0; nivel < 3 && !p.empty(); ++nivel) {
            candidatas.push_back(p);
            p = p.parent_path();
        }
    }
    for (const auto& c : candidatas) {
        if (fs::exists(c / "assets" / "models", ec) && fs::exists(c / "shaders", ec)) {
            g_base = c.u8string();
            return true;
        }
    }
    return false;
}

const std::string& base() { return g_base; }

std::string asset(const std::string& relativa) {
    return (fs::u8path(g_base) / "assets" / fs::u8path(relativa)).u8string();
}

std::string shader(const std::string& nombre) {
    return (fs::u8path(g_base) / "shaders" / fs::u8path(nombre)).u8string();
}


}  // namespace rutas
