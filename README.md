# Texturizado, Color e Iluminación — Escena de Día de Muertos

**Proyecto de Computación Gráfica e Interacción Humano-Computadora · Equipo 8**  
Universidad Nacional Autónoma de México · Facultad de Ingeniería · Semestre 2027-1

## Descripción

Proyecto de visualización 3D en tiempo real, desarrollado en **C++17 y OpenGL 3.3**, que representa una escena inspirada en la celebración mexicana del **Día de Muertos**. Esta etapa transforma los modelos geométricos de una entrega previa mediante texturas, materiales, transparencias, un entorno atmosférico y un sistema de iluminación dinámica.

La escena integra una ofrenda, un camino empedrado, flores de cempasúchil, papel picado, cirios y otros elementos decorativos. El proyecto busca combinar calidad visual, eficiencia de renderizado y ejecución multiplataforma.

## Características principales

- **Materiales y texturas:** mapas de color y rugosidad, así como mapeo de normales con cálculo de derivadas (`dFdx` y `dFdy`) en el *fragment shader*.
- **Flujo de trabajo procedural:** creación y horneado de materiales en Blender; exportación de modelos mediante **glTF 2.0**.
- **Iluminación híbrida:** luz direccional lunar y hasta **64 luces puntuales** con atenuación cuadrática para cirios y faroles.
- **Transparencias y materiales ópticos:** recorte alfa para papel picado y pétalos, y aproximaciones de transmitancia y Fresnel para vidrio y líquidos.
- **Skybox:** entorno panorámico con ajuste de luminancia para crear una atmósfera de transición hacia la noche.
- **Optimización geométrica:** reducción de polígonos, instanciación de elementos texturizados y generación procedural de coordenadas UV para el camino.
- **Compatibilidad multiplataforma:** integración y pruebas documentadas en macOS, Windows 10, Arch Linux y Linux Mint.

> **Alcance:** esta entrega no incluye animación esquelética compleja, *shadow mapping* de alta resolución ni reflejos mediante trazado de rayos en tiempo real.

## Tecnologías

| Tecnología | Uso |
| --- | --- |
| **C++17** | Implementación de la aplicación y lógica de la escena |
| **OpenGL 3.3 / GLSL** | Renderizado, texturizado e iluminación |
| **Blender** | Modelado, materiales procedurales y horneado de texturas |
| **glTF 2.0** | Intercambio de modelos y materiales |
| **CMake** | Configuración y compilación multiplataforma |
| **GLFW, GLAD, GLM y cgltf** | Ventana y contexto gráfico, acceso a OpenGL, matemáticas y lectura de glTF |

Las dependencias GLFW, GLAD, GLM y cgltf están incluidas en el proyecto, según la documentación de los lanzadores. Se requieren las herramientas de compilación y bibliotecas del sistema correspondientes.

## Ejecución

El repositorio incluye lanzadores en [`ejecutables/`](ejecutables/) que automatizan la configuración y compilación **Release**, preparan los recursos `assets/` y `shaders/` y abren la aplicación.

### macOS

1. Instala **CMake** y las herramientas de línea de comandos de **Xcode**.
2. Abre `ejecutables/macOS/TexturizadoCempasuchil-0.2.1.app`.
3. Si macOS bloquea la aplicación por no estar firmada, haz clic derecho sobre ella y selecciona **Abrir**.

### Windows

1. Instala **CMake** y **Visual Studio 2022** con la carga de trabajo **Desarrollo para el escritorio con C++**.
2. Abre `ejecutables/windows/TexturizadoCempasuchil.vbs`.

### Linux

1. Instala **CMake**, un compilador de C++ y las bibliotecas de desarrollo de **X11/OpenGL** que requiere GLFW.
2. Marca `ejecutables/linux/TexturizadoCempasuchil.desktop` como ejecutable o confiable y ábrelo. Alternativamente, ejecuta `ejecutables/linux/iniciar_linux.sh`.

**Diagnóstico:** los lanzadores guardan registros en `logs/`. Para requisitos, rutas de los registros y solución de problemas, consulta [la guía de los lanzadores](ejecutables/README.md).

## Controles de la escena

| Tecla o acción | Función |
| --- | --- |
| `W` `A` `S` `D` | Desplazarse por la escena |
| `Q` / `E` | Descender / ascender |
| Botón derecho + movimiento del ratón | Orientar la cámara |
| `Shift` | Acelerar el movimiento |
| Rueda del ratón | Ajustar la velocidad |
| `F2` | Alternar entre vista sólida y de alambre |
| `R` | Volver a la posición inicial |
| `Esc` | Salir |

## Organización del proyecto

Estas son algunas rutas relevantes para la etapa de texturizado e iluminación:

```text
.
├── assets/
│   ├── models/propios/         # Modelos propios exportados en glTF
│   ├── textures/               # Mapas de textura y materiales
│   └── skybox/atardecer/       # Recursos del entorno panorámico
├── shaders/
│   ├── escena.vert             # Transformaciones y datos de los vértices
│   └── escena.frag             # Materiales, normales e iluminación
├── src/render/
│   ├── Shader.cpp              # Gestión y compilación de shaders
│   └── Skybox.cpp              # Renderizado del entorno
├── ejecutables/                # Lanzadores para macOS, Windows y Linux
└── doc/
    ├── esp/                    # Reporte en español
    └── eng/                    # Reporte en inglés
```

## Documentación técnica

Para consultar el desarrollo, la metodología Scrum, las decisiones de implementación, los retos multiplataforma y los resultados visuales:

- **[Reporte técnico en español (PDF)](doc/esp/Equipo-8-texturizado-esp.pdf)**
- **[Technical report in English (PDF)](doc/eng/Equipo-8-texturizado-eng.pdf)**

## Equipo de desarrollo

| Integrante | Rol |
| --- | --- |
| Andrés Basilio Illescas | Product Owner |
| Emmanuel Trejo Olvera | Scrum Master |
| Alexis Abraham Pérez Olvera | Developer |
| Samuel Santiago Cruz Macedo | Developer |

**Profesor:** Ing. José Ramón Pérez Athié  
**Asignatura:** Computación Gráfica e Interacción Humano-Computadora · Grupo 04

---

*Proyecto académico de la Facultad de Ingeniería, UNAM. Inspirado en las tradiciones del Día de Muertos.*

