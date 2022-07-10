#version 300 es

precision highp float;

uniform sampler2D u_tex0;
uniform float u_sample_count;
uniform float u_exposure;
uniform float u_gamma;

in vec2 uv;

out vec4 outColor;

//https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 tonemap(vec3 v)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((v*(a*v+b))/(v*(c*v+d)+e), 0.0f, 1.0f);
}

void main()
{
	vec4 v = texture(u_tex0, uv);
    //v.rgb /= u_sample_count;
    v.rgb = tonemap(v.rgb * u_exposure);
    v.rgb = pow(v.rgb, vec3(1.0/u_gamma));
    outColor.rgb = v.rgb; 
	outColor.a = 1.0;
}