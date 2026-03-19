#version 330

// Full-screen triangle trick — no vertex data needed
// gl_VertexID: 0,1,2

uniform mat4 uInvViewProj;  // inverse(proj * view)
uniform vec3 uCameraPos;

out vec3 vNearPoint;
out vec3 vFarPoint;

vec3 unproject(float x, float y, float z)
{
    vec4 p = uInvViewProj * vec4(x, y, z, 1.0);
    return p.xyz / p.w;
}

void main()
{
    // Two triangles covering clip space
    vec2 positions[6] = vec2[6](
        vec2(-1,-1), vec2( 1,-1), vec2( 1, 1),
        vec2(-1,-1), vec2( 1, 1), vec2(-1, 1)
    );
    vec2 p = positions[gl_VertexID];
    vNearPoint = unproject(p.x, p.y, -1.0);
    vFarPoint  = unproject(p.x, p.y, 1.0);
    gl_Position = vec4(p, 0.0, 1.0);
}
