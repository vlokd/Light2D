#version 300 es

precision highp float;

#define PI 3.1415926538

#define NUM_SAMPLES {codegen_samples_per_pixel}
#define MAX_TRACE_STEPS 10
#define MAX_TRACE_DST {codegen_miss_dst}
#define TRACE_HIT_EPS {codegen_hit_dst}
#define MAX_TRACE_RAYS {codegen_max_rays_per_sample}
#define POLYGON_POINTS_MAX 16

uniform float u_random_seed;
uniform vec2 u_tex0_size;

{codegen_uniforms}

in vec2 uv;
out vec4 outColor;

struct Ray
{
	vec2 o;
	vec2 d;
};

struct Material
{
    float emission;
    float refraction;
    float absorption;
    float pad0;
};

struct TraceResult
{
	float dst;
	float emission;
	float refractionIndex;
    float absorption;
};

float rand(vec2 uv) {
	return fract(sin(dot(uv, vec2(12.9898, 78.233))) * 43758.5453);
}

float Reflectance(vec2 i, vec2 normal, float n1n2)
{
    float n1 = 1.0;
    float n2 = n1n2;
	float cosI = dot(i, normal);;
	float sinF2 = n1n2 * n1n2 * (1.0 - cosI * cosI);

	if (sinF2 > 1.0)
	{
		return 1.0;
	}
	else
	{
		float cosF = sqrt(1.0 - sinF2);
        cosI = abs(cosI);
		float r1 = (n1 * cosF - n2 * cosI) / (n1 * cosF + n2 * cosI);
		float r2 = (n2 * cosI - n1 * cosF) / (n2 * cosI + n1 * cosF);
		float r = (r1 * r1 + r2 * r2) * 0.5;
		return r;
	}
}

float BeerLambert(float absorption, float dst)
{
    return exp(-absorption * dst);
}

float CircleSDF(vec2 pt, float circleRadius)
{
	return length(pt) - circleRadius;
}

float RectangleSDF(vec2 pt, vec2 halfSize, float rounding)
{
	vec2 pa = abs(pt);
	vec2 ph = pa - halfSize;
	vec2 d = max(ph, 0.0);
	return length(d) + min(max(ph.x, ph.y), 0.0) - rounding;
}

float PolygonSDF(vec2 pt, int ptsCount, vec2[POLYGON_POINTS_MAX] pts, float rounding)
{
    int i = 0;
    int j = ptsCount - 1;

    float minDst = 1000.f;
    float s = 1.0;

    while (i < ptsCount)
    {
        vec2 e = pts[i] - pts[j];
        vec2 p = pt - pts[j];
        vec2 perp = p - e * clamp(dot(p, e) / dot(e, e), 0.0, 1.0);
        float d = dot(perp, perp);
        minDst = min(d, minDst);
        bvec3 c = bvec3(pt.y >= pts[j].y, pt.y < pts[i].y , e.x * p.y - e.y * p.x > 0.0);
        if (all(c) || all(not(c)) ) 
        {
           s*=-1.0;  
        }

        j = i;
        i++;
    }
    float dstSign = s;
    return dstSign * sqrt(minDst) - rounding;
}
float AnnularSDF(float sdf, float radius)
{
    return abs(sdf) - radius;
}

float UnionSDF(float a, float b)
{
    return min(a, b);
}
float DifferenceSDF(float a, float b)
{
    return max(a, -b);
}
float IntersectionSDF(float a, float b)
{
    return max(a, b);
}

vec2 Rotate(vec2 pt, float angle)
{
    angle = -angle;
    vec2 res;
    res.x = cos(angle) * pt.x - sin(angle) * pt.y;
    res.y = sin(angle) * pt.x + cos(angle) * pt.y;
    return res;
}

TraceResult TraceUnion(TraceResult a, TraceResult b)
{
	if (a.dst < b.dst)
	{
		return a;
	}
	else
	{
		return b;
	}
}
TraceResult TraceIntersection(TraceResult a, TraceResult b)
{
	if (a.dst >= b.dst)
	{
		return a;
	}
	else
	{
		return b;
	}
}
TraceResult TraceDifference(TraceResult a, TraceResult b)
{
    b.dst = -b.dst;
	if (a.dst >= b.dst)
	{
		return a;
	}
	else
	{
		return b;
	}
}

bool DoesRayIntersectAABB(vec2 origin, vec2 dir, vec4 aabb)
{
    float dx = 1.0 / (dir.x);
    float tx0 = (aabb.x - origin.x) * dx;
    float tx1 = (aabb.z - origin.x) * dx;
    float txmin = min(tx0, tx1);
    float txmax = max(tx0, tx1);

    float dy = 1.0 / (dir.y);
    float ty0 = (aabb.y - origin.y) * dy;
    float ty1 = (aabb.w - origin.y) * dy;
    float tymin = min(ty0, ty1);
    float tymax = max(ty0, ty1);

    float tmin = max(txmin, tymin);
    float tmax = min(txmax, tymax);

    return tmax >= tmin;
}

{codegen_scene}

float ScenePartDeriv(vec2 pt, vec2 dir, vec2 rayDir)
{
	float eps = 0.0001;
	float res = TraceScene(pt + eps * dir, rayDir).dst - TraceScene(pt - eps * dir, rayDir).dst;
	res = res * 0.5 / eps;
	return res;
}

vec2 SceneNormal(vec2 pt, vec2 rayDir)
{
	float dfdx = ScenePartDeriv(pt, vec2(1, 0), rayDir);
	float dfdy = ScenePartDeriv(pt, vec2(0, 1), rayDir);
	return normalize(vec2(dfdx, dfdy));
}

float TraceRayCycled(Ray r)
{
    float rayPath = 0.f;
	Ray rc = r;
	float t = 0.0;
	TraceResult traceRes;
	float totalEmission = 0.0;
	float emissionMult = 1.0;

    int rayIdx = 0;
    int stepIdx = 0;

	while (rayIdx < MAX_TRACE_RAYS)
	{
		while (stepIdx < MAX_TRACE_STEPS && t < MAX_TRACE_DST)
		{
			vec2 cp = rc.o + rc.d * t;
			traceRes = TraceScene(cp, rc.d);
			float sdfSign = (traceRes.dst >= 0.0) ? 1.0 : -1.0;
			if (traceRes.dst * sdfSign < TRACE_HIT_EPS)
			{
                totalEmission += traceRes.emission * emissionMult;
                if (sdfSign < 0.f)
                {
                    emissionMult *= BeerLambert(traceRes.absorption, t + traceRes.dst * sdfSign);
                }
                if (traceRes.refractionIndex > 0.0)
                {
                    vec2 normal = SceneNormal(cp, rc.d) * sdfSign;
                    float n1n2 = (sdfSign > 0.0) ? (1.0 / traceRes.refractionIndex) : traceRes.refractionIndex;
                    float reflectance = Reflectance(rc.d, normal, n1n2);
                    vec2 refracted = refract(rc.d, normal, n1n2);
                    //refract
                    if (dot(refracted, refracted) > 0.5)
                    {
                        if (rand(rc.o + rc.d + u_random_seed) <= (1.0 - reflectance))
					    {
						    rc.o = cp;
						    rc.d = refracted;
                            rc.o += -1.f * TRACE_HIT_EPS * normal * 2.0;

						    t = 0.0;
						    stepIdx = MAX_TRACE_STEPS;

						    break;
					    }
                    }
                    //reflect
				    {
					    rc.o = cp;
					    rc.d = reflect(rc.d, normal);
					    rc.o += rc.d * TRACE_HIT_EPS * 2.0;

					    t = 0.0;
					    stepIdx = MAX_TRACE_STEPS;
                    
					    break;
				    }
                }
				else
				{
                    t = 0.0;
					stepIdx = MAX_TRACE_STEPS;
					rayIdx = MAX_TRACE_RAYS;
					break;
				}
			}
			else
			{
				t += traceRes.dst * sdfSign;
                stepIdx++;
			}
		}
        stepIdx = 0;
        rayIdx++;
	}
	return totalEmission;
}

void main()
{
    vec2 texelSize = 1.f / u_tex0_size;
	float ar = u_tex0_size.x / u_tex0_size.y;
    vec2 uvc = gl_FragCoord.xy / u_tex0_size;
	uvc = uvc * 2.0 - 1.0;
    if (ar > 1.f)
    {
        uvc.x *= ar;
    }
    else
    {
        uvc.y /= ar;
    }

    outColor = vec4(0, 0, 0, 1);

	float v = 0.0;

    //TODO: Proper sampling
	float angularStep = PI * 2.0 / float(NUM_SAMPLES);
    for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        vec2 offset = vec2(rand(vec2(u_random_seed + float(i), 0.f)), rand(vec2(u_random_seed + float(i), 1.f)));
        offset = offset * 2.f - 1.f;
        vec2 coord = uvc + offset * texelSize * 1.f;

        Ray r;
        r.o = coord;
        float angle = angularStep * (float(i) + rand(uvc + u_random_seed));
        r.d.x = cos(angle);
        r.d.y = sin(angle);
        v += TraceRayCycled(r);
    }

	v /= float(NUM_SAMPLES);
    outColor.rgb = vec3(v);
}