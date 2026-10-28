#version 330 core
// Texturizado de ambiente con cube map (skybox).

in vec3 vDireccion;

uniform samplerCube uCielo;
uniform float uExposicion;

out vec4 FragColor;

void main() {
    vec3 c = texture(uCielo, normalize(vDireccion)).rgb * uExposicion;  // muestreo sRGB -> lineal
    FragColor = vec4(pow(c, vec3(1.0 / 2.2)), 1.0);
}
