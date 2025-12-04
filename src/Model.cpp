#include "Model.hpp"
#include <map>
#include "ObjReader.hpp"
#include "Transform.hpp"
#include "animate.hpp"

Model::Model(std::string objFile, std::string mtlFile, float scale, std::string name, ModelType type, bool shadows)
	: triangles{readObjFile(objFile, mtlFile, scale)}, name{name}, type{type}, shadows{shadows} {
	if (type == MIRROR_PHONG || type == SMOOTH_GOURAUD || type == SMOOTH_PHONG || type == GLASS_PHONG)
		calculateVertexNormals();
	else
		calculateFaceNormals();
}

void Model::calculateFaceNormals() {
	for (ModelTriangle& triangle : triangles) {
		triangle.normal = glm::normalize(glm::cross(triangle.vertices[1] - triangle.vertices[0], triangle.vertices[2] - triangle.vertices[0]));
	}
}

void Model::calculateVertexNormals() {
	// key: vertex (unique), value: sum of face normals (of each face that uses the vertex)
	std::map<glm::vec3, glm::vec3, vec3Compare> vertexNormalSums;
	for (ModelTriangle& triangle : triangles) {
		// compute face normal
		triangle.normal = glm::normalize(glm::cross(triangle.vertices[1] - triangle.vertices[0], triangle.vertices[2] - triangle.vertices[0]));

		// add face normal to vertex sum
		vertexNormalSums[triangle.vertices[0]] += triangle.normal;
		vertexNormalSums[triangle.vertices[1]] += triangle.normal;
		vertexNormalSums[triangle.vertices[2]] += triangle.normal;
	}
	for (ModelTriangle& triangle : triangles) {
		// find average of all face normals by normalising
		triangle.vertexNormals[0] = glm::normalize(vertexNormalSums[triangle.vertices[0]]);
		triangle.vertexNormals[1] = glm::normalize(vertexNormalSums[triangle.vertices[1]]);
		triangle.vertexNormals[2] = glm::normalize(vertexNormalSums[triangle.vertices[2]]);
	}
}

void Model::addTransformation(TransformationType type, float x, float y, float z, float seconds, int parallel) {
	float fps = 15;
	if (type == TRANSLATE || type == ROTATE || type == SCALE) {
		for (int i = 1; i <= seconds * fps; i++) {
			if (parallel == 0) transformations0.push_back(Transformation{type, (x) / (seconds * fps), (y) / (seconds * fps), (z) / (seconds * fps)});
			if (parallel == 1) transformations1.push_back(Transformation{type, (x) / (seconds * fps), (y) / (seconds * fps), (z) / (seconds * fps)});
			if (parallel == 2) transformations2.push_back(Transformation{type, (x) / (seconds * fps), (y) / (seconds * fps), (z) / (seconds * fps)});
		}
	}
	if ((type >= FLAT_ && type <= LIGHT_) || type == SHADOWS) {
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

void Model::translate(float x, float y, float z) {
	glm::vec3 translation{x, y, z};
	for (ModelTriangle& triangle : triangles) {
		// FIXME: do we need to transform normals here??
		// triangle.normal = translation;
		triangle.vertices[0] += translation;
		triangle.vertices[1] += translation;
		triangle.vertices[2] += translation;
		// if (type == MIRROR_PHONG || type == SMOOTH_GOURAUD || type == SMOOTH_PHONG || type == GLASS_PHONG){
		//     triangle.vertexNormals[0] += translation;
		//     triangle.vertexNormals[1] += translation;
		//     triangle.vertexNormals[2] += translation;
		// }
	}
}
void Model::rotate(float xDegrees, float yDegrees, float zDegrees) {
	for (ModelTriangle& triangle : triangles) {
		triangle.normal = glm::normalize(Rotate3(xDegrees, yDegrees, zDegrees) * triangle.normal);
		triangle.vertices[0] = Rotate3(xDegrees, yDegrees, zDegrees) * triangle.vertices[0];
		triangle.vertices[1] = Rotate3(xDegrees, yDegrees, zDegrees) * triangle.vertices[1];
		triangle.vertices[2] = Rotate3(xDegrees, yDegrees, zDegrees) * triangle.vertices[2];
		if (type == MIRROR_PHONG || type == SMOOTH_GOURAUD || type == SMOOTH_PHONG || type == GLASS_PHONG) {
			triangle.vertexNormals[0] = glm::normalize(Rotate3(xDegrees, yDegrees, zDegrees) * triangle.vertexNormals[0]);
			triangle.vertexNormals[1] = glm::normalize(Rotate3(xDegrees, yDegrees, zDegrees) * triangle.vertexNormals[1]);
			triangle.vertexNormals[2] = glm::normalize(Rotate3(xDegrees, yDegrees, zDegrees) * triangle.vertexNormals[2]);
		}
	}
}
void Model::scale(float x, float y, float z) {
	glm::vec3 scaling{x, y, z};
	for (ModelTriangle& triangle : triangles) {
		// triangle.normal *= scaling;
		triangle.vertices[0] *= scaling;
		triangle.vertices[1] *= scaling;
		triangle.vertices[2] *= scaling;
		// if (type == MIRROR_PHONG || type == SMOOTH_GOURAUD || type == SMOOTH_PHONG || type == GLASS_PHONG){
		//     triangle.vertexNormals[0] *= scaling;
		//     triangle.vertexNormals[1] *= scaling;
		//     triangle.vertexNormals[2] *= scaling;
		// }
	}
}
// void Model::translate(float x, float y, float z){
//     translation.x += x;
//     translation.y += y;
//     translation.z += z;
// }
// void Model::rotate(float xDegrees, float yDegrees, float zDegrees){
//     rotation.x += xDegrees;
//     while (rotation.x >= 360) rotation.x -= 360;
//     rotation.y += yDegrees;
//     while (rotation.y >= 360) rotation.y -= 360;
//     rotation.z += zDegrees;
//     while (rotation.z >= 360) rotation.z -= 360;
// }
// void Model::scale(float x, float y, float z){
//     scaling.x += x;
//     scaling.y += y;
//     scaling.z += z;
// }
// void Model::applyTransformation(){
//     for (ModelTriangle& triangle : triangles){
//         if (translation.x != 0 || translation.y != 0 || translation.z != 0){
//             triangle.normal += translation;
//             triangle.vertices[0] += translation;
//             triangle.vertices[1] += translation;
//             triangle.vertices[2] += translation;
//             if (type == MIRROR_PHONG || type == SMOOTH_GOURAUD || type == SMOOTH_PHONG || type == GLASS_PHONG){
//                 triangle.vertexNormals[0] += translation;
//                 triangle.vertexNormals[1] += translation;
//                 triangle.vertexNormals[2] += translation;
//             }
//             translation.x = 0;
//             translation.y = 0;
//             translation.z = 0;
//         }
//         if (rotation.x != 0 || rotation.y != 0 || rotation.z != 0){
//             triangle.normal = Rotate3(rotation.x, rotation.y, rotation.z) * triangle.normal;
//             triangle.vertices[0] = Rotate3(rotation.x, rotation.y, rotation.z) * triangle.vertices[0];
//             triangle.vertices[1] = Rotate3(rotation.x, rotation.y, rotation.z) * triangle.vertices[1];
//             triangle.vertices[2] = Rotate3(rotation.x, rotation.y, rotation.z) * triangle.vertices[2];
//             if (type == MIRROR_PHONG || type == SMOOTH_GOURAUD || type == SMOOTH_PHONG){
//                 triangle.vertexNormals[0] = Rotate3(rotation.x, rotation.y, rotation.z) * triangle.vertexNormals[0];
//                 triangle.vertexNormals[1] = Rotate3(rotation.x, rotation.y, rotation.z) * triangle.vertexNormals[1];
//                 triangle.vertexNormals[2] = Rotate3(rotation.x, rotation.y, rotation.z) * triangle.vertexNormals[2];
//             }
//             rotation.x = 0;
//             rotation.y = 0;
//             rotation.z = 0;
//         }
//         if (scaling.x != 1 || scaling.y != 1 || scaling.z != 1){
//             triangle.normal *= scaling;
//             triangle.vertices[0] *= scaling;
//             triangle.vertices[1] *= scaling;
//             triangle.vertices[2] *= scaling;
//             if (type == MIRROR_PHONG || type == SMOOTH_GOURAUD || type == SMOOTH_PHONG || type == GLASS_PHONG){
//                 triangle.vertexNormals[0] *= scaling;
//                 triangle.vertexNormals[1] *= scaling;
//                 triangle.vertexNormals[2] *= scaling;
//             }
//             scaling.x = 1;
//             scaling.y = 1;
//             scaling.z = 1;
//         }
//     }
// }
