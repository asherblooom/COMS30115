#include "Camera.hpp"

void Camera::lookat(float x, float y, float z){
    glm::vec3 forward = glm::normalize(position - glm::vec3{x, y, z});
    glm::vec3 right = glm::normalize(glm::cross({0, 1, 0}, forward));
    glm::vec3 up = glm::normalize(glm::cross(forward, right));
    orientation = {right, up, forward};
}

void Camera::translate(float x, float y, float z){
    position.x += x;
    position.y += y;
    position.z += z;
}
void Camera::rotate(float xDegrees, float yDegrees, float zDegrees){
    orientation = Rotate3(xDegrees, yDegrees, zDegrees) * orientation;
}
void Camera::rotatePosition(float xDegrees, float yDegrees, float zDegrees){
    position = Rotate3(xDegrees, yDegrees, zDegrees) * position;
}

void Camera::addTransformation(TransformationType type, float x, float y, float z, float seconds, int parallel){
    float fps = 10;
    if (type == TRANSLATE || type == ROTATE || type == ROTATEPOSITION){
        for (int i = 1; i <= seconds * fps; i++){ 
            if (parallel == 0) transformations0.push_back(Transformation{type, (x)/(seconds*fps), (y)/(seconds*fps), (z)/(seconds*fps)});
            if (parallel == 1) transformations1.push_back(Transformation{type, (x)/(seconds*fps), (y)/(seconds*fps), (z)/(seconds*fps)});
            if (parallel == 2) transformations2.push_back(Transformation{type, (x)/(seconds*fps), (y)/(seconds*fps), (z)/(seconds*fps)});
        }
    }
    if (type == WAIT){
        for (int i = 1; i <= seconds * fps; i++){ 
            if (parallel == 0) transformations0.push_back(Transformation{type, 0,0,0});
            if (parallel == 1) transformations1.push_back(Transformation{type, 0,0,0});
            if (parallel == 2) transformations2.push_back(Transformation{type, 0,0,0});
        }
    }
}