// Entrega independiente de modelado: recorrido y vista de alambre.
#include "App.h"
#include "Registro.h"
#include "Rutas.h"

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>

int main(int argc, char** argv) {
    Opciones op;
    bool verificar=false;
    for(int i=1;i<argc;++i) {
        const std::string argumento=argv[i];
        if(argumento=="--comprobar")op.comprobar=true;
        if(argumento=="--altar")op.vistaInicial=3;
        if(argumento=="--detalle-altar")op.vistaInicial=4;
        if(argumento=="--verificar-apoyos")verificar=true;
    }
    if (!rutas::inicializar(argc > 0 ? argv[0] : nullptr)) {
        registro::fatal("No se encontraron las carpetas assets/ y shaders/ junto al programa.");
        return 1;
    }
    registro::abrir((std::filesystem::u8path(rutas::base()) / "registro.txt").u8string());
    registro::info("Carpeta base: %s", rutas::base().c_str());

    if(verificar) {
        Modelo::soloCPU=true;
        Escena escena;bool ok=false;
        try {ok=escena.cargar();} catch(const std::exception& e){registro::error("Validación: %s",e.what());}
        escena.liberar();registro::info("Validación CPU: %s",ok?"correcta":"fallida");registro::cerrar();return ok?0:1;
    }
    App app;
    int codigo = app.ejecutar(op);
    registro::cerrar();
    return codigo;
}
