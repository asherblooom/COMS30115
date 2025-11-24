#ifndef LIGHT_HPP
#define LIGHT_HPP

#include <glm/glm.hpp>

class Light{
public:
    float strength;
    float ambientStrength;
    glm::vec3 position;

    Light(float strength, float ambientStrength) : strength{strength}, ambientStrength{ambientStrength}, position{0} {}

    void translate(float x, float y, float z);
    void rotate(float xDegrees, float yDegrees, float zDegrees);
};

#endif