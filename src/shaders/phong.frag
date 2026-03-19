#version 330

in vec3 vNormal;
in vec3 vFragPos;

uniform vec3  uLightDir;    // direction TO the light, world space
uniform vec3  uLightColor;
uniform vec3  uObjectColor;
uniform vec3  uViewPos;
uniform float uAmbient;

out vec4 fragColor;

void main()
{
    // Ambient
    vec3 ambient = uAmbient * uLightColor;

    // Diffuse
    vec3  n    = normalize(vNormal);
    float diff = max(dot(n, normalize(uLightDir)), 0.0);
    vec3  diffuse = diff * uLightColor;

    // Specular (Blinn-Phong)
    vec3  viewDir  = normalize(uViewPos - vFragPos);
    vec3  halfDir  = normalize(normalize(uLightDir) + viewDir);
    float spec     = pow(max(dot(n, halfDir), 0.0), 32.0);
    vec3  specular = spec * 0.3 * uLightColor;

    vec3 result = (ambient + diffuse + specular) * uObjectColor;
    fragColor = vec4(result, 1.0);
}
