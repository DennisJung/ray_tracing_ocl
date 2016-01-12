// Simple RayTracing Header
// Reference https://github.com/Tecla/Rayito
//
#ifndef __RAYTRACING_H__
#define __RAYTRACING_H__

namespace RAYTRACING
{


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

typedef struct SphereSet{
	RectangleLight* m_rectLight;
	int LightCount;
	Plane* m_plane;
	int PlaneCount;
}SphereSet;

typedef struct Camera{
	float fieldOfViewInDegrees;
	Point origin;
	Vector target;
	Vector targetUpDirection;
}Camera;

}

#endif
