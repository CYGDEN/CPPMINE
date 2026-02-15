#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 sunDir = normalize(vec3(-0.4, -0.7, -0.5));
    vec3 sunColor = vec3(1.0, 0.9, 0.75);
    float sunIntensity = 1.2;
    vec3 ambientColor = vec3(0.4, 0.45, 0.55);
    float ambientIntensity = 0.25;
    vec3 fogColor = vec3(0.35, 0.38, 0.42);
    float fogStart = 30.0;
    float fogEnd = 150.0;

    vec3 N = normalize(fragNormal);

    float ndotl = max(dot(N, -sunDir), 0.0);

    float shadow = 1.0;
    float terminator = smoothstep(0.0, 0.15, ndotl);
    shadow *= terminator;

    vec3 diffuse = sunColor * ndotl * sunIntensity * shadow;

    float upFactor = N.y * 0.5 + 0.5;
    vec3 skyAmbient = fogColor * upFactor * 0.15;
    vec3 ambient = ambientColor * ambientIntensity + skyAmbient;

    vec3 groundBounce = vec3(0.1, 0.08, 0.05) * max(0.0, -N.y) * 0.1;

    float edgeDark = 1.0;
    vec3 absN = abs(N);
    float minComp = min(absN.x, min(absN.y, absN.z));
    if (minComp < 0.1) edgeDark = 0.85;

    vec3 totalLight = diffuse + ambient + groundBounce;

    vec3 result = fragColor * totalLight * edgeDark;

    float gamma = 1.0 / 2.2;
    result = pow(result, vec3(gamma));

    float dist = length(fragWorldPos);
    float fogFactor = clamp((dist - fogStart) / (fogEnd - fogStart), 0.0, 1.0);
    fogFactor = fogFactor * fogFactor;
    result = mix(result, fogColor, fogFactor);

    outColor = vec4(result, 1.0);
}