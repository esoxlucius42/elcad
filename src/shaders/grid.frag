#version 330

in vec3 vNearPoint;
in vec3 vFarPoint;

uniform float uNear;
uniform float uFar;
uniform mat4  uProj;
uniform mat4  uView;

out vec4 fragColor;

// Analytical infinite grid (Aras Pranckevičius technique)
vec4 grid(vec3 fragPos, float scale, bool drawAxis)
{
    vec2 coord = fragPos.xy * scale;
    vec2 deriv = fwidth(coord);
    vec2 grid  = abs(fract(coord - 0.5) - 0.5) / deriv;
    float line = min(grid.x, grid.y);
    float minY = min(deriv.y, 1.0);
    float minX = min(deriv.x, 1.0);

    vec4 color = vec4(0.4, 0.4, 0.4, 1.0 - min(line, 1.0));

    if (drawAxis) {
        // Y axis — green
        if (abs(fragPos.x) < 0.08 * minX)
            color = mix(color, vec4(0.2, 1.0, 0.2, 1.0), 0.8);
        // X axis — red
        if (abs(fragPos.y) < 0.08 * minY)
            color = mix(color, vec4(1.0, 0.2, 0.2, 1.0), 0.8);
    }
    return color;
}

float computeDepth(vec3 pos)
{
    vec4 clip = uProj * uView * vec4(pos, 1.0);
    return (clip.z / clip.w) * 0.5 + 0.5;
}

float computeLinearDepth(vec3 pos)
{
    vec4 clip  = uProj * uView * vec4(pos, 1.0);
    float ndcZ = clip.z / clip.w;          // [-1, 1]
    float linearDepth = (2.0 * uNear * uFar) / (uFar + uNear - ndcZ * (uFar - uNear));
    return linearDepth / uFar;
}

void main()
{
    float t = -vNearPoint.z / (vFarPoint.z - vNearPoint.z);
    if (t < 0.0) discard;

    vec3 fragPos = vNearPoint + t * (vFarPoint - vNearPoint);

    gl_FragDepth = computeDepth(fragPos);

    float linearDepth  = computeLinearDepth(fragPos);
    float fading       = max(0.0, 0.5 - linearDepth);

    fragColor  = grid(fragPos, 0.1, true) * float(t > 0.0);
    fragColor.a *= fading;
    if (fragColor.a < 0.002) discard;
}
