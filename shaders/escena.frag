#version 330 core
in vec3 vNormal;
in vec3 vPosicion;
in vec2 vUV;
in vec4 vTinte;

uniform int uAlambre;
uniform vec3 uOjo;
uniform vec4 uColorBase;
uniform vec3 uEmision;
uniform float uAutoEmision;
uniform float uMetalico;
uniform float uRugosidad;
uniform float uEscalaNormal;
uniform float uOclusion;
uniform float uEspecular;
uniform float uTransmision;
uniform float uIor;
uniform int uTipoTransparente;
uniform float uCorteAlfa;
uniform int uMascara;
uniform int uSinLuz;
uniform int uTieneColor;
uniform int uTieneMetalRugosidad;
uniform int uTieneNormal;
uniform int uTieneOclusion;
uniform int uTieneEmision;
uniform sampler2D uTexturaColor;
uniform sampler2D uTexturaMetalRugosidad;
uniform sampler2D uTexturaNormal;
uniform sampler2D uTexturaOclusion;
uniform sampler2D uTexturaEmision;

struct LuzPuntual {
    vec3 posicion;
    vec3 color;
    float lineal;
    float cuadratica;
    float radio2;
    float farol;
};
const int MAX_LUCES = 64;
uniform int uCantidadLuces;
uniform LuzPuntual uLuces[MAX_LUCES];
uniform vec3 uDireccionLuna;
uniform vec3 uColorLuna;
uniform vec3 uAmbienteNoche;
uniform int uSpotActivo;
uniform int uRecibeLucesPuntuales;
uniform vec3 uSpotPosicion;
uniform vec3 uSpotDireccion;

out vec4 FragColor;

vec3 normalMapeada(vec3 base) {
    if (uTieneNormal == 0) return base;
    vec3 dpdx = dFdx(vPosicion), dpdy = dFdy(vPosicion);
    vec2 duvdx = dFdx(vUV), duvdy = dFdy(vUV);
    float det = duvdx.x * duvdy.y - duvdx.y * duvdy.x;
    if (abs(det) < 1e-8) return base;
    vec3 tangente = normalize((dpdx * duvdy.y - dpdy * duvdx.y) / det);
    tangente = normalize(tangente - base * dot(base, tangente));
    vec3 bitangente = normalize(cross(base, tangente)) * (det < 0.0 ? -1.0 : 1.0);
    vec3 mapa = texture(uTexturaNormal, vUV).xyz * 2.0 - 1.0;
    mapa.xy *= uEscalaNormal;
    return normalize(mat3(tangente, bitangente, base) * mapa);
}

void main() {
    if (uAlambre != 0) { FragColor = vec4(0.8, 0.8, 0.8, 1.0); return; }
    vec4 base = uColorBase * vTinte;
    if (uTieneColor != 0) base *= texture(uTexturaColor, vUV);
    if (uMascara != 0 && base.a < uCorteAlfa) discard;

    vec2 mr = vec2(uMetalico, uRugosidad);
    if (uTieneMetalRugosidad != 0) {
        vec4 texMR = texture(uTexturaMetalRugosidad, vUV);
        mr *= vec2(texMR.b, texMR.g); // glTF: B=metallic, G=roughness
    }
    mr = clamp(mr, vec2(0.0), vec2(1.0));

    vec3 normal = normalize(vNormal);
    if (!gl_FrontFacing) normal = -normal;
    normal = normalMapeada(normal);
    vec3 emision = uEmision;
    if (uTieneEmision != 0) emision *= texture(uTexturaEmision, vUV).rgb;
    // El papel picado conserva su color en penumbra sin comportarse como una
    // fuente de luz real ni iluminar los objetos vecinos.
    emision += base.rgb * uAutoEmision;
    if (uSinLuz != 0) {
        FragColor = vec4(pow(max(base.rgb + emision, vec3(0.0)), vec3(1.0 / 2.2)), base.a);
        return;
    }

    vec3 vista = normalize(uOjo - vPosicion);
    float oclusion = uTieneOclusion != 0
        ? mix(1.0, texture(uTexturaOclusion, vUV).r, uOclusion) : 1.0;
    vec3 f0 = mix(vec3(0.04 * uEspecular), base.rgb, mr.x);
    float brillo = mix(128.0, 4.0, mr.y);

    // Luna: direccional azul tenue, sin la antigua iluminación solar fija.
    vec3 luzLuna = normalize(uDireccionLuna);
    float ndlLuna = max(dot(normal, luzLuna), 0.0);
    vec3 medioLuna = normalize(luzLuna + vista);
    float specLuna = pow(max(dot(normal, medioLuna), 0.0), brillo) * ndlLuna;
    vec3 color = base.rgb * uAmbienteNoche * oclusion;
    color += base.rgb * (1.0 - mr.x) * uColorLuna * ndlLuna;
    color += f0 * uColorLuna * specLuna * mix(1.0, 0.35, mr.y);

    // Veladoras, cirios y faroles: alcance corto mediante atenuación cuadrática.
    if (uRecibeLucesPuntuales != 0) {
        for (int i = 0; i < MAX_LUCES; ++i) {
            if (i >= uCantidadLuces) break;
            vec3 delta = uLuces[i].posicion - vPosicion;
            float distancia2 = dot(delta, delta);
            if (distancia2 > uLuces[i].radio2) continue;
            float distancia = sqrt(distancia2);
            vec3 L = delta / max(distancia, 0.0001);
            // Los faroles iluminan en todas direcciones; su radio y atenuación
            // limitan suavemente el alcance alrededor de la estructura.
            float conoFarol = 1.0;
            float bordeRadio = 1.0;
            if (uLuces[i].farol > 0.5) {
                float radio = sqrt(uLuces[i].radio2);
                bordeRadio = 1.0 - smoothstep(radio * 0.75, radio, distancia);
            }
            float ndl = max(dot(normal, L), 0.0);
            float atenuacion = 1.0 / (1.0 + uLuces[i].lineal * distancia +
                                     uLuces[i].cuadratica * distancia * distancia);
            vec3 medio = normalize(L + vista);
            float spec = pow(max(dot(normal, medio), 0.0), brillo) * ndl;
            color += (base.rgb * (1.0 - mr.x) * ndl + f0 * spec * mix(1.0, 0.35, mr.y))
                     * uLuces[i].color * atenuacion * conoFarol * bordeRadio;
        }
    }

    // Linterna opcional de la cámara, con cono suave y atenuación propia.
    if (uSpotActivo != 0) {
        vec3 desdeSpot = vPosicion - uSpotPosicion;
        float distancia = length(desdeSpot);
        vec3 rayo = desdeSpot / max(distancia, 0.0001);
        float theta = dot(rayo, normalize(uSpotDireccion));
        float cono = smoothstep(cos(radians(13.0)), cos(radians(9.0)), theta);
        vec3 L = -rayo;
        float ndl = max(dot(normal, L), 0.0);
        float atenuacion = 1.0 / (1.0 + 0.09 * distancia + 0.032 * distancia * distancia);
        vec3 medio = normalize(L + vista);
        float spec = pow(max(dot(normal, medio), 0.0), brillo) * ndl;
        vec3 luzSpot = vec3(0.95, 0.80, 0.62) * cono * atenuacion;
        color += (base.rgb * (1.0 - mr.x) * ndl + f0 * spec) * luzSpot;
    }

    color += emision;
    float frente = max(dot(normal, vista), 0.0);
    float f0Vidrio = pow((uIor - 1.0) / (uIor + 1.0), 2.0);
    float fresnel = f0Vidrio + (1.0 - f0Vidrio) * pow(1.0 - frente, 5.0);
    if (uTransmision > 0.0) {
        // Aproximación sin refracción de pantalla: conserva el tinte del vidrio,
        // refuerza cantos y reflejos y permite leer los objetos que hay detrás.
        float aporteColor = uTipoTransparente == 2 ? 0.62 : 0.16;
        vec3 transmitido = base.rgb * (aporteColor + 0.18 * ndlLuna) +
                           vec3(fresnel * (0.75 + 0.25 * uEspecular));
        color = mix(color, transmitido, clamp(uTransmision, 0.0, 1.0));
    }
    color = color / (color + vec3(1.0));
    float alfaBase = uTipoTransparente == 2 ? 0.34 : 0.10;
    float alfaVidrio = clamp(alfaBase + fresnel * 0.72 + (1.0 - mr.y) * 0.08, 0.08, 0.92);
    float alfa = base.a * mix(1.0, alfaVidrio, clamp(uTransmision, 0.0, 1.0));
    FragColor = vec4(pow(max(color, vec3(0.0)), vec3(1.0 / 2.2)), alfa);
}
