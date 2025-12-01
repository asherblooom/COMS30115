#ifndef ANIMATE_HPP
#define ANIMATE_HPP

enum TransformationType{
	// model types
	FLAT_,
	FLAT_SPECULAR_,
	SMOOTH_GOURAUD_,
	SMOOTH_PHONG_,
	MIRROR_,
	MIRROR_PHONG_,
	GLASS_,
	GLASS_PHONG_,
	LIGHT_,
	// light types
	POINTLIGHT,
	AREALIGHT,
	// others
	ROTATE,
	SCALE,
	TRANSLATE,
	ROTATEPOSITION,
	WAIT,
	SWITCH_RENDERING_METHOD
};

struct Transformation {
	TransformationType type;
	float x;
	float y;
	float z;
};

#endif