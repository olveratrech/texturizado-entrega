#version 330 core
in vec3 vNormal;
in vec3 vPosicion;
in vec2 vUV;
in vec4 vTinte;

uniform int uAlambre;
uniform vec3 uOjo;
uniform vec4 uColorBase;
uniform vec3 uEmision;
uniform float uMetalico;
uniform float uRugosidad;
uniform float uEscalaNormal;
uniform float uOclusion;
uniform float uEspecular;
uniform float uTransmision;
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
    if (uSinLuz != 0) {
        FragColor = vec4(pow(max(base.rgb + emision, vec3(0.0)), vec3(1.0 / 2.2)), base.a);
        return;
    }

    vec3 luz = normalize(vec3(-0.55, 0.8, 0.55));
    vec3 vista = normalize(uOjo - vPosicion);
    vec3 medio = normalize(luz + vista);
    float ndl = max(dot(normal, luz), 0.0);
    float ndh = max(dot(normal, medio), 0.0);
    float oclusion = uTieneOclusion != 0
        ? mix(1.0, texture(uTexturaOclusion, vUV).r, uOclusion) : 1.0;
    vec3 f0 = mix(vec3(0.04 * uEspecular), base.rgb, mr.x);
    float brillo = mix(128.0, 4.0, mr.y);
    vec3 ambiente = base.rgb * mix(0.24, 0.12, mr.x) * oclusion;
    vec3 difusa = base.rgb * (1.0 - mr.x) * ndl * 0.78;
    vec3 especular = f0 * pow(ndh, brillo) * ndl * mix(1.0, 0.35, mr.y);
    vec3 color = ambiente + difusa + especular + emision;
    color = color / (color + vec3(1.0));
    float alfa = base.a * (1.0 - 0.75 * clamp(uTransmision, 0.0, 1.0));
    FragColor = vec4(pow(max(color, vec3(0.0)), vec3(1.0 / 2.2)), alfa);
}
