#include "Transform.hpp"

glm::mat4 Rotate(float xDegrees, float yDegrees, float zDegrees) {
	glm::mat4 x{1};
	glm::mat4 y{1};
	glm::mat4 z{1};
	if (xDegrees != 0) {
		float cosX = std::cos(xDegrees * (M_PI / 180.0f));
		float sinX = std::sin(xDegrees * (M_PI / 180.0f));
		x = {1, 0, 0, 0, 0, cosX, -sinX, 0, 0, sinX, cosX, 0, 0, 0, 0, 1};
	}
	if (yDegrees != 0) {
		float cosY = std::cos(yDegrees * (M_PI / 180.0f));
		float sinY = std::sin(yDegrees * (M_PI / 180.0f));
		y = {cosY, 0, sinY, 0, 0, 1, 0, 0, -sinY, 0, cosY, 0, 0, 0, 0, 1};
	}
	if (zDegrees != 0) {
		float cosZ = std::cos(zDegrees * (M_PI / 180.0f));
		float sinZ = std::sin(zDegrees * (M_PI / 180.0f));
		z = {cosZ, sinZ, 0, 0, -sinZ, cosZ, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
	}
	return x * y * z;
}

glm::mat4 Translate(float x, float y, float z) {
	return glm::mat4{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, y, z, 1};
}

glm::mat4 Scale(float x, float y, float z) {
	return {x, 0, 0, 0, 0, y, 0, 0, 0, 0, z, 0, 0, 0, 0, 1};
}

glm::mat3 Rotate3(float xDegrees, float yDegrees, float zDegrees) {
	glm::mat3 x{1};
	glm::mat3 y{1};
	glm::mat3 z{1};
	if (xDegrees != 0) {
		float cosX = std::cos(xDegrees * (M_PI / 180.0f));
		float sinX = std::sin(xDegrees * (M_PI / 180.0f));
		x = {1, 0, 0, 0, cosX, -sinX, 0, sinX, cosX};
	}
	if (yDegrees != 0) {
		float cosY = std::cos(yDegrees * (M_PI / 180.0f));
		float sinY = std::sin(yDegrees * (M_PI / 180.0f));
		y = {cosY, 0, sinY, 0, 1, 0, -sinY, 0, cosY};
	}
	if (zDegrees != 0) {
		float cosZ = std::cos(zDegrees * (M_PI / 180.0f));
		float sinZ = std::sin(zDegrees * (M_PI / 180.0f));
		z = {cosZ, sinZ, 0, -sinZ, cosZ, 0, 0, 0, 1};
	}
	return x * y * z;
}

glm::mat3 Scale3(float x, float y, float z) {
	return {x, 0, 0, 0, y, 0, 0, 0, z};
}