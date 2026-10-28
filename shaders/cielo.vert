#version 330 core
// Skybox: cubo unitario centrado en la cámara. Se elimina la traslación de la
// matriz de vista y se fuerza z = w para que quede en la profundidad máxima.

layout(location = 0) in vec3 aPos;

uniform mat4 uVistaSinTraslacion;
uniform mat4 uProyeccion;

out vec3 vDireccion;

void main() {
    vDireccion = aPos;
    vec4 p = uProyeccion * uVistaSinTraslacion * vec4(aPos, 1.0);
    gl_Position = p.xyww;
}
