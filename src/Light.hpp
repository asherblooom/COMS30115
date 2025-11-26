#ifndef LIGHT_HPP
#define LIGHT_HPP

#include <glm/glm.hpp>

enum LightType {
    POINT,
    AREA
};

class Light{
public:
    float strength;
    float ambientStrength;
    glm::vec3 position;
    LightType type;

    int uSize;
    int vSize;
    glm::vec3 uVec;
    glm::vec3 vVec;

    Light(float strength, float ambientStrength, LightType type) 
    : strength{strength}, ambientStrength{ambientStrength}, position{0}, type{type} {}

    Light(float strength, float ambientStrength, LightType type, int uSize, int vSize, glm::vec3 uVec, glm::vec3 vVec) 
    : strength{strength}, ambientStrength{ambientStrength}, position{0}, type{type}, uSize{uSize}, vSize{vSize}, uVec{uVec}, vVec{vVec} {}

    void translate(float x, float y, float z);
    void rotate(float xDegrees, float yDegrees, float zDegrees);
};

#endif