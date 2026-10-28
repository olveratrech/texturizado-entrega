#version 330 core
in vec3 vNormal;
uniform int uAlambre;
out vec4 FragColor;
void main() {
    if (uAlambre != 0) { FragColor = vec4(0.8, 0.8, 0.8, 1.0); return; }
    vec3 normal = normalize(vNormal);
    if (!gl_FrontFacing) normal = -normal;
    vec3 luz = normalize(vec3(-0.55, 0.8, 0.55));
    float difusa = max(dot(normal, luz), 0.0);
    // Luz ambiental y una luz direccional blanca: superficie gris mate.
    float gris = pow(0.55 * (0.28 + 0.72 * difusa), 1.0 / 2.2);
    FragColor = vec4(vec3(gris), 1.0);
}
