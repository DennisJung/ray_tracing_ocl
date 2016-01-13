
#ifdef __APPLE__
#define OCL_CONSTANT_BUFFER __global
#else
#define OCL_CONSTANT_BUFFER __constant
#endif

#include "define.h"

#define RAYMAX  1.0e30f
#define EPSILON 0.00001f

#ifndef M_PI
  // For some reason, MSVC doesn't define this when <cmath> is included
  #define M_PI 3.14159265358979
#endif

typedef struct Color{
	float x, y, z;
} Color;

typedef struct Vector{
	float x, y, z;
} Vector;

typedef struct Vector Point;

typedef struct Ray{
	Point m_origin;
	Vector m_direction;
	float m_tMax;
}Ray;

typedef struct RectangleLight{
	Point m_pos;
	Vector m_side1, m_side2;
	Color m_color;
	float m_power;
}RectangleLight;

typedef struct Plane{
	Point m_pos;
	Vector m_normal;
	Color m_color; 
}Plane;

typedef struct Camera{
	float fieldOfViewInDegrees;
    Point origin;
    Vector target;
    Vector targetUpDirection;
}Camera;

typedef struct Intersection{ 
	Ray m_ray;
	Color m_color;
	Color m_emitted;
	Vector m_normal;
	float m_t;
	int lastindex;
}Intersection;

#define vinit(v, a, b, c) { (v).x = a; (v).y = b; (v).z = c; }
#define vassign(a, b) vinit(a, (b).x, (b).y, (b).z)
#define vclr(v) vinit(v, 0.f, 0.f, 0.f)
#define vadd(v, a, b) vinit(v, (a).x + (b).x, (a).y + (b).y, (a).z + (b).z)
#define vsub(v, a, b) vinit(v, (a).x - (b).x, (a).y - (b).y, (a).z - (b).z)
#define vsadd(v, a, b) { float k = (a); vinit(v, (b).x + k, (b).y + k, (b).z + k) }
#define vssub(v, a, b) { float k = (a); vinit(v, (b).x - k, (b).y - k, (b).z - k) }
#define vmul(v, a, b) vinit(v, (a).x * (b).x, (a).y * (b).y, (a).z * (b).z)
#define vsmul(v, a, b) { float k = (a); vinit(v, k * (b).x, k * (b).y, k * (b).z) }
#define vdiv(v, a, b) vinit(v, (a).x / (b).x, (a).y / (b).y, (a).z / (b).z)
#define vsdiv(v, a, b) { float k = (a); vinit(v, (b).x / k, (b).y / k, (b).z / k) }
#define vdot(a, b) ((a).x * (b).x + (a).y * (b).y + (a).z * (b).z)
#define vnorm(v) { float l = 1.f / sqrt(vdot(v, v)); vsmul(v, l, v); }
#define vxcross(v, a, b) vinit(v, (a).y * (b).z - (a).z * (b).y, (a).z * (b).x - (a).x * (b).z, (a).x * (b).y - (a).y * (b).x)
#define vfilter(v) ((v).x > (v).y && (v).x > (v).z ? (v).x : (v).y > (v).z ? (v).y : (v).z)
#define viszero(v) (((v).x == 0.f) && ((v).x == 0.f) && ((v).z == 0.f))
#define pcal(v, a, b, c) { vsmul(v, a, c); vadd(v, v, b);}

#define vclamp(v) { vinit(v, clamp((v).x, 0.0f, 1.0f), clamp((v).y, 0.0f, 1.0f), clamp((v).z, 0.0f, 1.0f))}

static float GetRandom(unsigned int *seed0, unsigned int *seed1) {
	*seed0 = 36969 * ((*seed0) & 65535) + ((*seed0) >> 16);
	*seed1 = 18000 * ((*seed1) & 65535) + ((*seed1) >> 16);

	unsigned int ires = ((*seed0) << 16) + (*seed1);

	return ((*seed0 << 16) + *seed1) * 2.328306e-10f;
}

static Ray makeCameraRay(OCL_CONSTANT_BUFFER const Camera* cam, float xScreenPosTo1, float yScreenPosTo1) {
	Vector forward, right, up, tmp;
	Ray ray;

	vsub(tmp, cam->target, cam->origin); vnorm(tmp); vassign(forward, tmp);
	vxcross(tmp, forward, cam->targetUpDirection); vnorm(tmp); vassign(right, tmp);
	vxcross(tmp, right, forward); vnorm(tmp); vassign(up, tmp);

	float tanFov = tan(cam->fieldOfViewInDegrees * M_PI / 180.0f);
	
	vsmul(right, ((xScreenPosTo1 - 0.5f) * tanFov), right);
	vsmul(up, ((yScreenPosTo1 - 0.5f) * tanFov), up);
	
	ray.m_origin = cam->origin;
	vadd(ray.m_direction, forward, right);
	vadd(ray.m_direction, ray.m_direction, up);
	vnorm(ray.m_direction);
	ray.m_tMax = RAYMAX;
	return ray;
}

static bool RectangleLightIntersect(RectangleLight tmpRectangle, int index,Intersection* tmpIntersection)
{
	Vector normal;
	vxcross(normal, tmpRectangle.m_side1, tmpRectangle.m_side2); vnorm(normal);
	
	
	float nDotD = vdot(normal, tmpIntersection->m_ray.m_direction);
	if (nDotD == 0.0f)
	{
		return false;
	}

	float t = (vdot(tmpRectangle.m_pos, normal) - vdot(tmpIntersection->m_ray.m_origin, normal)) / 
		(vdot(tmpIntersection->m_ray.m_direction, normal));
	
	if(t >= tmpIntersection->m_t || t < EPSILON)
	{
		return false; 
	}
	
	Vector side1Norm = tmpRectangle.m_side1;
	Vector side2Norm = tmpRectangle.m_side2;
	float side1Length = sqrt(vdot(side1Norm, side1Norm)); vnorm(side1Norm);
	float side2Length = sqrt(vdot(side2Norm, side2Norm)); vnorm(side2Norm);
	
	Vector worldPoint, worldRelativePoint, localPoint;
	pcal(worldPoint, t, tmpIntersection->m_ray.m_origin, tmpIntersection->m_ray.m_direction);
	
	vsub(worldRelativePoint, worldPoint, tmpRectangle.m_pos);
	
	
	vinit(localPoint, vdot(worldRelativePoint, side1Norm), 
		vdot(worldRelativePoint, side2Norm), 0.0f);
	
	if((localPoint.x < 0.0f) || (localPoint.x > side1Length) ||
		(localPoint.y < 0.0f) || (localPoint.y > side2Length))
	{
		return false;
	}
	
	
	tmpIntersection->m_t = t;
	tmpIntersection->lastindex = index;
	tmpIntersection->m_normal = normal;
	vclr(tmpIntersection->m_color);
	vsmul(tmpIntersection->m_emitted, tmpRectangle.m_power, tmpRectangle.m_color);
	
	if((vdot(tmpIntersection->m_normal, tmpIntersection->m_ray.m_direction)) > 0.0f)
	{
		vsmul(tmpIntersection->m_normal, -1.0f, tmpIntersection->m_normal);
	}
	
	return true;
}

static bool PlaneIntersect(Plane tmpPlane, Intersection* tmpIntersection)
{
	float nDotD = vdot(tmpPlane.m_normal, tmpIntersection->m_ray.m_direction);
;	if (nDotD >= 0.0f)
	{
		return false;
	}
	

	float t = (vdot(tmpPlane.m_pos, tmpPlane.m_normal) - 
		vdot(tmpIntersection->m_ray.m_origin, tmpPlane.m_normal)) / 
		(vdot(tmpIntersection->m_ray.m_direction, tmpPlane.m_normal));
	
	if(t >= tmpIntersection->m_t || t < EPSILON)
	{
		return false; 
	}
	

	tmpIntersection->m_t = t;
	tmpIntersection->m_normal = tmpPlane.m_normal;
	vclr(tmpIntersection->m_emitted);
	tmpIntersection->m_color = tmpPlane.m_color;

	return true;
}

static bool intersect(Intersection* tmpIntersection, 
	OCL_CONSTANT_BUFFER const RectangleLight* lights,
	const unsigned int lightcount, OCL_CONSTANT_BUFFER const Plane* planes,
	const unsigned int planecount)
{
	bool intersectedAny = false;
	int i;

	for(i = 0; i<planecount; i++)
	{
		if(PlaneIntersect(planes[i], tmpIntersection) )
		{
			intersectedAny = true;
			
			
		}
	}
	
	for(i = 0; i<lightcount; i++)
	{
		if (RectangleLightIntersect(lights[i], i, tmpIntersection))
		{
			intersectedAny = true;
		}
		
	}

	return intersectedAny;
}

static bool sampleSurface(RectangleLight tmpRectangle, float u1, float u2,
	const Point* referencePosition, Point* outPosition, Vector* outNormal)
{
	Point tmp;
	vxcross(tmp, tmpRectangle.m_side1, tmpRectangle.m_side2); vnorm(tmp); vassign(*outNormal, tmp);

	vsmul(tmp, u1, tmpRectangle.m_side1); vadd(*outPosition, tmpRectangle.m_pos, tmp);

	vsmul(tmp, u2, tmpRectangle.m_side2); vadd(*outPosition, *outPosition, tmp);

	vsub(tmp, *outPosition, *referencePosition);

	if (vdot(*outNormal, tmp) > 0.0f)
	{
		vsmul(*outNormal, -1.0f, *outNormal);
	}

	return true;
}

// TODO: Add OpenCL kernel code here.
__kernel void ray_cal(OCL_CONSTANT_BUFFER const RectangleLight* lights,
	const unsigned int lightcount, OCL_CONSTANT_BUFFER const Plane* planes,
	const unsigned int planecount, const unsigned int sampleCount,
	const unsigned int width, const unsigned int height,
	OCL_CONSTANT_BUFFER const Camera* cam, __global unsigned int* seeds,
	__global unsigned int* pixels, const unsigned int stage)
{
    const int offset     = get_global_id(0);
    const int y		= (stage * WORK_AMOUNT + offset) / WIDTH_SIZE;
	const int x     = (stage * WORK_AMOUNT + offset) % WIDTH_SIZE;
	unsigned int seed0, seed1;
	
	int i, j;
	
	seed0 = seeds[2*offset];
	seed1 = seeds[2*offset + 1];

	Color pixelColor;
	vclr(pixelColor);
	float yu, xu;
	//pixels[0] = pixels[0] + 1;
	pixels[y*width+x] = 0;
	for(i= 0; i< sampleCount; i++)
	{
		yu = 1.0f - ((y + GetRandom(&seed0, &seed1)) / (height - 1));
		xu = (x + GetRandom(&seed0, &seed1)) / (width - 1);
		
		Ray ray = makeCameraRay(cam, xu, yu);
		Intersection intersection;
		intersection.m_ray = ray;
		intersection.m_t = ray.m_tMax;
		vclr(intersection.m_color);
		vclr(intersection.m_emitted);
		vclr(intersection.m_normal);
		intersection.lastindex = -1;
		if(intersect(&intersection, lights, lightcount, planes, planecount))
		{
			vadd(pixelColor, pixelColor, intersection.m_emitted);

			Point position;
			pcal(position, intersection.m_t, intersection.m_ray.m_origin, 
				intersection.m_ray.m_direction);
			
			for(j = 0; j<lightcount; j++)
			{
				Point lightPoint;
				Vector lightNormal;

				sampleSurface(lights[j], GetRandom(&seed0, &seed1), GetRandom(&seed0, &seed1),
							&position, &lightPoint, &lightNormal);
				
				
				Vector toLight; vsub(toLight, lightPoint, position);
				float lightDistance = sqrt(vdot(toLight, toLight)); vnorm(toLight)
				Ray shadowRay = { position, toLight, lightDistance}; 
				Intersection shadowIntersection;
				shadowIntersection.m_ray = shadowRay;
				shadowIntersection.m_t = shadowRay.m_tMax;
				vclr(shadowIntersection.m_color);
				vclr(shadowIntersection.m_emitted);
				vclr(shadowIntersection.m_normal);
				shadowIntersection.lastindex = -1;
				bool intersected = intersect(&shadowIntersection, lights, lightcount, planes, planecount);

				if(!intersected || (shadowIntersection.lastindex == j))
				{
					float lightAttenuation = max(0.0f, vdot(intersection.m_normal, toLight));
					Color tmp;
					vsmul(tmp, lights[j].m_power, lights[j].m_color);
					vmul(tmp, intersection.m_color, tmp);
					vsmul(tmp, lightAttenuation, tmp);
				
					vadd(pixelColor, pixelColor, tmp);
				}

			}
			
			

		}
	}

	
	seeds[2*offset] = seed0;
	seeds[2*offset + 1] = seed1;

	vsdiv(pixelColor, sampleCount, pixelColor);
	
	vclamp(pixelColor);
	
	unsigned char r, g, b;
	
	r = (unsigned char)(pixelColor.x * 255.0f);
	g = (unsigned char)(pixelColor.y * 255.0f);
	b = (unsigned char)(pixelColor.z * 255.0f);

	pixels[y*width+x] = (r << 16) + (g << 8) + b;
	
}