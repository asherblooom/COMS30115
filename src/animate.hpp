#ifndef ANIMATE_HPP
#define ANIMATE_HPP

enum TransformationType{
	// model types
	FLAT,
	FLAT_SPECULAR,
	SMOOTH_GOURAUD,
	SMOOTH_PHONG,
	MIRROR,
	MIRROR_PHONG,
	GLASS,
	GLASS_PHONG,
	LIGHT,
	// light types
	POINT,
	AREA,
	// others
	ROTATE,
	SCALE,
	TRANSLATE,
	ROTATEPOSITION,
	WAIT,
};

struct Transformation {
	TransformationType type;
	float x;
	float y;
	float z;
};

#endif