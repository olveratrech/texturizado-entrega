#include <mach-o/dyld.h>
#include <limits.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {
    char ejecutable[PATH_MAX];
    uint32_t capacidad = sizeof(ejecutable);
    if (_NSGetExecutablePath(ejecutable, &capacidad) != 0) return 10;

    char copia[PATH_MAX];
    snprintf(copia, sizeof(copia), "%s", ejecutable);
    char *macos = dirname(copia);
    char script[PATH_MAX];
    snprintf(script, sizeof(script), "%s/../Resources/iniciar.zsh", macos);

    char **argumentos = calloc((size_t)argc + 2, sizeof(char *));
    if (!argumentos) return 11;
    argumentos[0] = "/bin/zsh";
    argumentos[1] = script;
    for (int i = 1; i < argc; ++i) argumentos[i + 1] = argv[i];
    execv("/bin/zsh", argumentos);
    perror("No se pudo iniciar el lanzador");
    return 12;
}
