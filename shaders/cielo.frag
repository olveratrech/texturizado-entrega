#version 330 core
// Texturizado de ambiente con cube map (skybox).

in vec3 vDireccion;

uniform samplerCube uCielo;

out vec4 FragColor;

void main() {
    vec3 c = texture(uCielo, normalize(vDireccion)).rgb;
    float luma = dot(c, vec3(0.2126, 0.7152, 0.0722));
    vec3 noche = mix(c * vec3(0.012, 0.028, 0.090),
                     vec3(luma) * vec3(0.018, 0.050, 0.160), 0.68);
    FragColor = vec4(pow(max(noche, vec3(0.0)), vec3(1.0 / 2.2)), 1.0);
}
