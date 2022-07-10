#version 300 es

precision highp float;

uniform sampler2D u_tex0;
uniform float u_sample_count;

in vec2 uv;

out vec4 outColor;

void main()
{
	vec4 v = texture(u_tex0, uv) / u_sample_count;
    outColor.rgb = v.rgb; 
	outColor.a = 1.0;
}