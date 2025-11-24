#ifndef MODEL_HPP
#define MODEL_HPP
#include <ModelTriangle.h>
#include "Transform.hpp"
#include <vector>

class Model {
    public:
    std::vector<ModelTriangle> triangles;
    std::string name;
    Model(std::vector<ModelTriangle> triangles, std::string name) : triangles{triangles}, name{name} {};
};

class Camera {
    public:
    float focalLength;
    glm::vec3 position;
    // col1: right, col2: up, col3: forward
    glm::mat3 orientation;
    Camera(float focalLength) : focalLength{focalLength}, position{0}, orientation{1} {};
    void lookat(float x, float y, float z){
        glm::vec3 forward = glm::normalize(position - glm::vec3{x, y, z});
        glm::vec3 right = glm::normalize(glm::cross({0, 1, 0}, forward));
        glm::vec3 up = glm::normalize(glm::cross(forward, right));
        orientation = {right, up, forward};
    }
    void translate(float x, float y, float z){
        position.x += x;
        position.y += y;
        position.z += z;
    }
    void rotate(float xDegrees, float yDegrees, float zDegrees){
        orientation = Rotate3(xDegrees, yDegrees, zDegrees) * orientation;
    }
    void rotatePosition(float xDegrees, float yDegrees, float zDegrees){
        position = Rotate3(xDegrees, yDegrees, zDegrees) * position;
    }
};

#endif