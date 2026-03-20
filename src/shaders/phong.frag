#version 330

in vec3 vNormal;
in vec3 vFragPos;

uniform vec3 uLightDir;    // direction TO the key light, world space
uniform vec3 uLightColor;
uniform vec3 uObjectColor;
uniform vec3 uViewPos;

// Hemisphere ambient
uniform vec3 uSkyColor;
uniform vec3 uGroundColor;

// Fill light (diffuse only)
uniform vec3 uFillDir;
uniform vec3 uFillColor;

out vec4 fragColor;

void main()
{
    vec3  n    = normalize(vNormal);

    // Hemisphere ambient: sky color for up-facing normals, ground for down-facing
    float t       = n.y * 0.5 + 0.5;
    vec3  ambient = mix(uGroundColor, uSkyColor, t);

    // Diffuse (key light)
    float diff = max(dot(n, normalize(uLightDir)), 0.0);
    vec3  diffuse = diff * uLightColor;

    // Fill light (diffuse only, no specular)
    float fillDiff    = max(dot(n, normalize(uFillDir)), 0.0);
    vec3  fillDiffuse = fillDiff * uFillColor;

    // Specular (Blinn-Phong, key light only)
    vec3  viewDir  = normalize(uViewPos - vFragPos);
    vec3  halfDir  = normalize(normalize(uLightDir) + viewDir);
    float spec     = pow(max(dot(n, halfDir), 0.0), 32.0);
    vec3  specular = spec * 0.3 * uLightColor;

    vec3 result = (ambient + diffuse + fillDiffuse + specular) * uObjectColor;
    fragColor = vec4(result, 1.0);
}
