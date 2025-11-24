#ifndef CAMERA_HPP
#define CAMERA_HPP
#include <glm/glm.hpp>
#include "Transform.hpp"

class Camera {
public:
    float focalLength;
    glm::vec3 position;
    // col1: right, col2: up, col3: forward
    glm::mat3 orientation;

    Camera(float focalLength) : focalLength{focalLength}, position{0}, orientation{1} {};

    void lookat(float x, float y, float z);
    void translate(float x, float y, float z);
    void rotate(float xDegrees, float yDegrees, float zDegrees);
    void rotatePosition(float xDegrees, float yDegrees, float zDegrees);
};

#endif