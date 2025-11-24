#ifndef MODEL_HPP
#define MODEL_HPP
#include "ModelTriangle.h"
#include "Transform.hpp"
#include "ObjReader.hpp"
#include <vector>
#include <map>

// vec3 comparator so I can use as key in map
struct vec3Compare {
    bool operator()(const glm::vec3 &a, const glm::vec3 &b) const {
        if (a.x != b.x) return a.x < b.x;
        if (a.y != b.y) return a.y < b.y;
        return a.z < b.z;
    }
};

class Model {
public:
    std::vector<ModelTriangle> triangles;
    std::string name;
    TriangleType type;
    bool shadows;

    Model(){}
    Model(std::string objFile, std::string mtlFile, float scale, std::string name, TriangleType type, bool shadows);

private:
    void calculateFaceNormals();
    void calculateVertexNormals();
};

#endif