#version 300 es

out highp vec2 uv;

void main() 
{
    vec2 positions[4] = vec2[](
        vec2(-1, -1),
        vec2(-1, 1),
        vec2(1, -1),
        vec2(1, 1)
    );
    vec2 uvs[4] = vec2[]
    (
        vec2(0.f, 0.f),
        vec2(0.f, 1.f),
        vec2(1.f, 0.f),
        vec2(1.f, 1.f)
    );
    gl_Position = vec4(positions[gl_VertexID], 0.5, 1.0);
    uv = uvs[gl_VertexID];
}