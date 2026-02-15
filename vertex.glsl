#version 450
layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec3 inColor;
layout(location=0) out vec3 voPosition;
layout(push_constant) uniform PushConsts { mat4 mvp; };
void main() {
    voPosition = inColor;
    gl_Position = mvp * vec4(inPosition, 1.0);
}