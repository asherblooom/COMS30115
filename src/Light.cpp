
#include "Light.hpp"
#include "Transform.hpp"

void Light::translate(float x, float y, float z) {
	position.x += x;
	position.y += y;
	position.z += z;
}

void Light::rotate(float xDegrees, float yDegrees, float zDegrees) {
	position = Rotate3(xDegrees, yDegrees, zDegrees) * position;
}

void Light::addTransformation(TransformationType type, float x, float y, float z, float seconds, int parallel) {
	float fps = 15;
	if (type == TRANSLATE || type == ROTATE) {
		for (int i = 1; i <= seconds * fps; i++) {
			if (parallel == 0) transformations0.push_back(Transformation{type, (x) / (seconds * fps), (y) / (seconds * fps), (z) / (seconds * fps)});
			if (parallel == 1) transformations1.push_back(Transformation{type, (x) / (seconds * fps), (y) / (seconds * fps), (z) / (seconds * fps)});
			if (parallel == 2) transformations2.push_back(Transformation{type, (x) / (seconds * fps), (y) / (seconds * fps), (z) / (seconds * fps)});
		}
	}
	if (type == POINTLIGHT || type == AREALIGHT) {
		if (parallel == 0) transformations0.push_back(Transformation{type, 0, 0, 0});
		if (parallel == 1) transformations1.push_back(Transformation{type, 0, 0, 0});
		if (parallel == 2) transformations2.push_back(Transformation{type, 0, 0, 0});
	}
	if (type == WAIT) {
		for (int i = 1; i <= seconds * fps; i++) {
			if (parallel == 0) transformations0.push_back(Transformation{type, 0, 0, 0});
			if (parallel == 1) transformations1.push_back(Transformation{type, 0, 0, 0});
			if (parallel == 2) transformations2.push_back(Transformation{type, 0, 0, 0});
		}
	}
}
