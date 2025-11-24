
#include "Light.hpp"
#include "Transform.hpp"

void Light::translate(float x, float y, float z){
    position.x += x;
    position.y += y;
    position.z += z;
}

void Light::rotate(float xDegrees, float yDegrees, float zDegrees){
    position = Rotate3(xDegrees, yDegrees, zDegrees) * position;
}