#version 450
layout(location=0) in vec3 voPosition;
layout(location=0) out vec4 outColor;
void main() {
    outColor = vec4(voPosition, 1.0);
}