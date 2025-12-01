#ifndef ANIMATE_HPP
#define ANIMATE_HPP
#include <functional>

enum TransformationType{
	ROTATE,
	SCALE,
	TRANSLATE,
	ROTATEPOSITION
};

struct Transformation {
	TransformationType type;
	float x;
	float y;
	float z;
};

#endif