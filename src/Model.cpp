#include "Model.hpp"

Model::Model(std::string objFile, std::string mtlFile, float scale, std::string name, TriangleType type, bool shadows)
    : triangles{readObjFile(objFile, mtlFile, scale)}, name{name}, type{type}, shadows{shadows} {
        if (type == MIRROR_PHONG || type == SMOOTH_GOURAUD || type == SMOOTH_PHONG)
            calculateVertexNormals();
        else
            calculateFaceNormals();
}

void Model::calculateFaceNormals(){
    for (ModelTriangle &triangle : triangles) {
        triangle.normal = glm::normalize(glm::cross(triangle.vertices[1] - triangle.vertices[0], triangle.vertices[2] - triangle.vertices[0]));
    }
}

void Model::calculateVertexNormals(){
    // key: vertex (unique), value: sum of face normals (of each face that uses the vertex)
    std::map<glm::vec3, glm::vec3, vec3Compare> vertexNormalSums;
    for (ModelTriangle &triangle : triangles) {
        // compute face normal
        triangle.normal = glm::normalize(glm::cross(triangle.vertices[1] - triangle.vertices[0], triangle.vertices[2] - triangle.vertices[0]));

        // add face normal to vertex sum
        vertexNormalSums[triangle.vertices[0]] += triangle.normal;
        vertexNormalSums[triangle.vertices[1]] += triangle.normal;
        vertexNormalSums[triangle.vertices[2]] += triangle.normal;
    }
    for (ModelTriangle &triangle : triangles) {
        // find average of all face normals by normalising
        triangle.vertexNormals[0] = glm::normalize(vertexNormalSums[triangle.vertices[0]]);
        triangle.vertexNormals[1] = glm::normalize(vertexNormalSums[triangle.vertices[1]]);
        triangle.vertexNormals[2] = glm::normalize(vertexNormalSums[triangle.vertices[2]]);
    }
}