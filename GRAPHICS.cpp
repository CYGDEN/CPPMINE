#pragma once

struct LightData {
    Vec3 sunDir;
    float sunIntensity;
    Vec3 sunColor;
    float ambientIntensity;
    Vec3 ambientColor;
    Vec3 fogColor;
    float fogStart;
    float fogEnd;
};

static LightData cityLight;

static void initLighting() {
    Vec3 sd = {-0.4f, -0.7f, -0.5f};
    cityLight.sunDir = sd.normalized();
    cityLight.sunIntensity = 1.2f;
    cityLight.sunColor = {1.0f, 0.9f, 0.75f};
    cityLight.ambientIntensity = 0.3f;
    cityLight.ambientColor = {0.4f, 0.45f, 0.55f};
    cityLight.fogColor = {0.35f, 0.38f, 0.42f};
    cityLight.fogStart = 40.0f;
    cityLight.fogEnd = 120.0f;
}

static Vec3 computeVertexLighting(Vec3 pos, Vec3 normal, Vec3 baseColor) {
    float ndotl = Vec3::dot(normal, Vec3{0,0,0} - cityLight.sunDir);
    if (ndotl < 0) ndotl = 0;
    Vec3 diffuse = cityLight.sunColor * (ndotl * cityLight.sunIntensity);
    Vec3 ambient = cityLight.ambientColor * cityLight.ambientIntensity;
    float upFactor = normal.y * 0.5f + 0.5f;
    Vec3 sky = cityLight.fogColor * (upFactor * 0.12f);
    return {
        clampf(baseColor.x * (diffuse.x + ambient.x + sky.x), 0, 1),
        clampf(baseColor.y * (diffuse.y + ambient.y + sky.y), 0, 1),
        clampf(baseColor.z * (diffuse.z + ambient.z + sky.z), 0, 1)
    };
}

static Vec3 applyFog(Vec3 color, float dist) {
    float f = clampf((dist - cityLight.fogStart) / (cityLight.fogEnd - cityLight.fogStart), 0, 1);
    f = f * f;
    return {
        color.x + (cityLight.fogColor.x - color.x) * f,
        color.y + (cityLight.fogColor.y - color.y) * f,
        color.z + (cityLight.fogColor.z - color.z) * f
    };
}

static Vec3 computeFullLighting(Vec3 pos, Vec3 normal, Vec3 baseColor, Vec3 eyePos) {
    Vec3 lit = computeVertexLighting(pos, normal, baseColor);
    float dist = (pos - eyePos).length();
    return applyFog(lit, dist);
}