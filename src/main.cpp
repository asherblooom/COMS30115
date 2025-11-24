#include <CanvasPoint.h>
#include <CanvasTriangle.h>
#include <Colour.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <ModelTriangle.h>
#include <RayTriangleIntersection.h>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <map>
#include "Draw.hpp"
#include "Model.hpp"
#include "ObjReader.hpp"
#include "Transform.hpp"

Colour castRay(std::vector<Model> &scene, glm::vec3 origin, glm::vec3 direction, glm::vec3 lightPos, float lightStrength, float ambientLightStrength, std::string originObjName = "", int depth = 0);

// vec3 comparator so I can use as key in map
struct vec3Compare {
	bool operator()(const glm::vec3 &a, const glm::vec3 &b) const {
		if (a.x != b.x) return a.x < b.x;
		if (a.y != b.y) return a.y < b.y;
		return a.z < b.z;
	}
};

void handleEvent(SDL_Event event, DrawingWindow &window, Camera& camera, bool &rasterising, bool &raytracedOnce) {
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_LEFT)
			camera.translate(0.25, 0, 0);
		if (event.key.keysym.sym == SDLK_RIGHT)
			camera.translate(-0.25, 0, 0);
		if (event.key.keysym.sym == SDLK_UP)
			camera.translate(0, -0.25, 0);
		if (event.key.keysym.sym == SDLK_DOWN)
			camera.translate(0, 0.25, 0);
		if (event.key.keysym.sym == SDLK_f)
			camera.translate(0, 0, -0.25);
		if (event.key.keysym.sym == SDLK_b)
			camera.translate(0, 0, 0.25);
		// cam rot
		if (event.key.keysym.sym == SDLK_w)
			camera.rotate(-1, 0, 0);
		if (event.key.keysym.sym == SDLK_s)
			camera.rotate(1, 0, 0);
		if (event.key.keysym.sym == SDLK_a)
			camera.rotate(0, -1, 0);
		if (event.key.keysym.sym == SDLK_d)
			camera.rotate(0, 1, 0);
		if (event.key.keysym.sym == SDLK_o)
			camera.rotate(0, 0, 1);
		if (event.key.keysym.sym == SDLK_p)
			camera.rotate(0, 0, -1);
		// mod rot
		if (event.key.keysym.sym == SDLK_l){
			camera.rotatePosition(0, -2, 0);
			camera.lookat(0, 0, 0);
		}
		if (event.key.keysym.sym == SDLK_h){
			camera.rotatePosition(0, 2, 0);
			camera.lookat(0, 0, 0);
		}
		if (event.key.keysym.sym == SDLK_k){
			camera.rotatePosition(2, 0, 0);
			camera.lookat(0, 0, 0);
		}
		if (event.key.keysym.sym == SDLK_j){
			camera.rotatePosition(-2, 0, 0);
			camera.lookat(0, 0, 0);
		}
		if (event.key.keysym.sym == SDLK_r) {
			if (rasterising) {
				rasterising = false;
				raytracedOnce = false;
			} else
				rasterising = true;
		}
	} else if (event.type == SDL_MOUSEBUTTONDOWN) {
		window.savePPM("output.ppm");
		window.saveBMP("output.bmp");
	}
}

CanvasPoint getCanvasIntersectionPoint(Camera& camera, glm::vec3 vertexPosition) {
	// glm::vec4 vertexPos{vertexPosition, 1};
	// vertexPos = modelRotation * vertexPos;
	// vertexPos -= cameraPosition;
	// vertexPos = cameraRotation * vertexPos;

	glm::vec3 cameraToVertex = vertexPosition - camera.position;
	glm::vec3 cameraToVertexRotated = cameraToVertex * camera.orientation;
	vertexPosition = cameraToVertexRotated;

	// FIXME: ask TA's about scaling model and funny behaviour when scale = 100, also about black lines appearing when rotating etc
	// also about boxes going through floor - is it ok??
	vertexPosition.x *= 100;
	// same reason as below for negative sign
	vertexPosition.y *= -100;
	// blue box has smaller z coord than red box, so need to flip (works because coords are relative to origin (middle) of object)
	vertexPosition.z *= -1;
	return {(vertexPosition.x * camera.focalLength) / vertexPosition.z + WIDTH / 2.0f, (vertexPosition.y * camera.focalLength) / vertexPosition.z + HEIGHT / 2.0f, vertexPosition.z};
}

CanvasTriangle getCanvasIntersectionTriangle(Camera& camera, ModelTriangle &triangle) {
	CanvasPoint v0 = getCanvasIntersectionPoint(camera, triangle.vertices[0]);
	CanvasPoint v1 = getCanvasIntersectionPoint(camera, triangle.vertices[1]);
	CanvasPoint v2 = getCanvasIntersectionPoint(camera, triangle.vertices[2]);
	return {v0, v1, v2};
}

void rasterise(DrawingWindow &window, std::vector<Model> &scene, Camera& camera) {
	// reset depth buffer
	std::vector<float> depthBuffer((window.width * window.height), 0);
	for (Model& model : scene){
		for (ModelTriangle &triangle : model.triangles) {
			bool draw = true;
			// transform camera????
			CanvasTriangle canvasTri = getCanvasIntersectionTriangle(camera, triangle);
			// dodgy culling
			for (CanvasPoint &vertexPos : canvasTri.vertices)
				if (vertexPos.x >= WIDTH || vertexPos.x < 0 || vertexPos.y >= HEIGHT || vertexPos.y < 0) draw = false;
			if (draw) drawFilledTriangle(window, depthBuffer, canvasTri, triangle.colour);
		}
	}
}

bool calculateShadows(std::vector<Model>& scene, glm::vec3 lightPos, glm::vec3 intersectionPoint, std::string originObjName) {
	glm::vec3 shadowRayDir = lightPos - intersectionPoint;
	float distToLight = glm::length(shadowRayDir);
	// we normalise after calculating distToLight so that distToLight is actual distance,
	// but also so that t == distToLight (as the vector which t scales - shadowRayDir - has length 1 after normalisation)
	shadowRayDir = glm::normalize(shadowRayDir);

	bool needShadow = false;
	float minT = distToLight;
	ModelTriangle hitTri;

	for (Model& model : scene){
		for (ModelTriangle &triangle : model.triangles) {
			if (triangle.objName == originObjName) continue; // don't want to hit ourselves
			glm::vec3 &v0 = triangle.vertices[0];
			glm::vec3 &v1 = triangle.vertices[1];
			glm::vec3 &v2 = triangle.vertices[2];

			glm::vec3 e0 = v1 - v0;
			glm::vec3 e1 = v2 - v0;
			glm::vec3 tuv = glm::inverse(glm::mat3(-shadowRayDir, e0, e1)) * (intersectionPoint - v0);
			float t = tuv.x;
			float u = tuv.y;
			float v = tuv.z;
			// t > 0.001 prevents shadow achne
			if (t >= 0.001 && u > 0 && v > 0 && u + v < 1) {
				// we hit a triangle
				if (t < minT) {
					minT = t;
					hitTri = triangle;
				}
			}
		}
	}
	if (minT < distToLight) {
		if (hitTri.shadows)
			needShadow = true;
	}
	return needShadow;
}

Colour ambientLightOnly(float ambientLightStrength, RayTriangleIntersection &intersection) {
	Colour colour = intersection.intersectedTriangle.colour;
	// add ambient light
	colour.red *= ambientLightStrength;
	colour.green *= ambientLightStrength;
	colour.blue *= ambientLightStrength;
	// Cap colour at (255, 255, 255)
	colour.red = std::min(colour.red, 255);
	colour.green = std::min(colour.green, 255);
	colour.blue = std::min(colour.blue, 255);

	return colour;
}

float diffuseAmbientMultiplier(glm::vec3 lightPos, float lightStrength, float ambientLightStrength, glm::vec3 intersectionPoint, glm::vec3 normal) {
	float diffuse = 0;
	float ambient = ambientLightStrength;

	float distanceTriToLight = glm::length(lightPos - intersectionPoint);
	glm::vec3 directionTriToLight = glm::normalize(lightPos - intersectionPoint);
	float dotProd = std::max(glm::dot(directionTriToLight, normal), 0.0f);
	diffuse = (lightStrength * dotProd) / (4 * M_PI * distanceTriToLight);

	return diffuse + ambient;
}

Colour diffuseAmbientShading(glm::vec3 lightPos, float lightStrength, float ambientLightStrength, glm::vec3 rayDir, RayTriangleIntersection &intersection) {
	Colour colour = intersection.intersectedTriangle.colour;
	glm::vec3 normal = intersection.intersectedTriangle.normal;
	glm::vec3 intersectionPoint = intersection.intersectionPoint;

	float light = diffuseAmbientMultiplier(lightPos, lightStrength, ambientLightStrength, intersectionPoint, normal);
	colour.red *= light;
	colour.green *= light;
	colour.blue *= light;

	// Cap colour at (255, 255, 255)
	colour.red = std::min(colour.red, 255);
	colour.green = std::min(colour.green, 255);
	colour.blue = std::min(colour.blue, 255);
	return colour;
}

float specularMultiplier(glm::vec3 lightPos, glm::vec3 intersectionPoint, glm::vec3 normal, glm::vec3 rayDir, int specExponent) {
	glm::vec3 directionTriToLight = glm::normalize(lightPos - intersectionPoint);
	glm::vec3 reflection = directionTriToLight - 2.0f * normal * glm::dot(directionTriToLight, normal);
	float specAngle = std::max(glm::dot(reflection, rayDir), 0.0f);
	float specular = std::pow(specAngle, specExponent);
	return specular;
}

Colour specularShading(glm::vec3 lightPos, float lightStrength, float ambientLightStrength, glm::vec3 rayDir, RayTriangleIntersection &intersection) {
	Colour colour = intersection.intersectedTriangle.colour;
	glm::vec3 normal = intersection.intersectedTriangle.normal;
	glm::vec3 intersectionPoint = intersection.intersectionPoint;

	// Add specular, assumes the light source is white (255, 255, 255)
	float specular = specularMultiplier(lightPos, intersectionPoint, normal, rayDir, 256);
	colour.red += (255.0f * specular);
	colour.green += (255.0f * specular);
	colour.blue += (255.0f * specular);

	// Cap colour at (255, 255, 255)
	colour.red = std::min(colour.red, 255);
	colour.green = std::min(colour.green, 255);
	colour.blue = std::min(colour.blue, 255);
	return colour;
}

Colour gouraudShading(glm::vec3 lightPos, float lightStrength, float ambientLightStrength, glm::vec3 rayDir, RayTriangleIntersection &intersection) {
	ModelTriangle triangle = intersection.intersectedTriangle;
	Colour colour = triangle.colour;
	float u = intersection.u;
	float v = intersection.v;
	float lightv0 = diffuseAmbientMultiplier(lightPos, lightStrength, ambientLightStrength, triangle.vertices[0], triangle.vertexNormals[0]);
	float lightv1 = diffuseAmbientMultiplier(lightPos, lightStrength, ambientLightStrength, triangle.vertices[1], triangle.vertexNormals[1]);
	float lightv2 = diffuseAmbientMultiplier(lightPos, lightStrength, ambientLightStrength, triangle.vertices[2], triangle.vertexNormals[2]);
	float lightPoint = (1 - u - v) * lightv0 + u * lightv1 + v * lightv2;

	float specv0 = specularMultiplier(lightPos, triangle.vertices[0], triangle.vertexNormals[0], rayDir, 16);
	float specv1 = specularMultiplier(lightPos, triangle.vertices[1], triangle.vertexNormals[1], rayDir, 16);
	float specv2 = specularMultiplier(lightPos, triangle.vertices[2], triangle.vertexNormals[2], rayDir, 16);
	float specPoint = (1 - u - v) * specv0 + u * specv1 + v * specv2;
	colour.red *= lightPoint;
	colour.green *= lightPoint;
	colour.blue *= lightPoint;
	colour.red += (255.0f * specPoint);
	colour.green += (255.0f * specPoint);
	colour.blue += (255.0f * specPoint);

	// Cap colour at (255, 255, 255)
	colour.red = std::min(colour.red, 255);
	colour.green = std::min(colour.green, 255);
	colour.blue = std::min(colour.blue, 255);
	return colour;
}

Colour phongShading(glm::vec3 lightPos, float lightStrength, float ambientLightStrength, glm::vec3 rayDir, RayTriangleIntersection &intersection) {
	ModelTriangle triangle = intersection.intersectedTriangle;
	Colour colour = triangle.colour;
	float u = intersection.u;
	float v = intersection.v;
	glm::vec3 intersectionNormal = (1 - u - v) * triangle.vertexNormals[0] + u * triangle.vertexNormals[1] + v * triangle.vertexNormals[2];
	intersectionNormal = glm::normalize(intersectionNormal);
	float lightPoint = diffuseAmbientMultiplier(lightPos, lightStrength, ambientLightStrength, intersection.intersectionPoint, intersectionNormal);
	float specPoint = specularMultiplier(lightPos, intersection.intersectionPoint, intersectionNormal, rayDir, 16);
	colour.red *= lightPoint;
	colour.green *= lightPoint;
	colour.blue *= lightPoint;
	colour.red += (255.0f * specPoint);
	colour.green += (255.0f * specPoint);
	colour.blue += (255.0f * specPoint);

	// Cap colour at (255, 255, 255)
	colour.red = std::min(colour.red, 255);
	colour.green = std::min(colour.green, 255);
	colour.blue = std::min(colour.blue, 255);
	return colour;
}

Colour mirror(std::vector<Model> &scene, glm::vec3 lightPos, float lightStrength, float ambientLightStrength, glm::vec3 rayDir, RayTriangleIntersection &intersection, int depth, int maxDepth, bool inShadow) {
	Colour colour;
	if (depth < maxDepth) {
		depth += 1;
		glm::vec3 reflection = glm::normalize(rayDir - 2.0f * intersection.intersectedTriangle.normal * glm::dot(rayDir, intersection.intersectedTriangle.normal));
		colour = castRay(scene, intersection.intersectionPoint, reflection, lightPos, lightStrength, ambientLightStrength, intersection.intersectedTriangle.objName, depth);
	}
	if (!inShadow){
		// calculate specular highlight
		float specular = specularMultiplier(lightPos, intersection.intersectionPoint, intersection.intersectedTriangle.normal, rayDir, 512);
		colour.red += (255.0f * specular);
		colour.green += (255.0f * specular);
		colour.blue += (255.0f * specular);
	}

	// mirrors don't always reflect 100% of light - here we pretend they only reflect 80%
	colour.red *= 0.8;
	colour.green *= 0.8;
	colour.blue *= 0.8;

	// Cap colour at (255, 255, 255)
	colour.red = std::min(colour.red, 255);
	colour.green = std::min(colour.green, 255);
	colour.blue = std::min(colour.blue, 255);
	return colour;
}

Colour phongMirror(std::vector<Model> &scene, glm::vec3 lightPos, float lightStrength, float ambientLightStrength, glm::vec3 rayDir, RayTriangleIntersection &intersection, int depth, int maxDepth, bool inShadow) {
	Colour colour;
	float u = intersection.u;
	float v = intersection.v;
	ModelTriangle &triangle = intersection.intersectedTriangle;
	glm::vec3 intersectionNormal = (1 - u - v) * triangle.vertexNormals[0] + u * triangle.vertexNormals[1] + v * triangle.vertexNormals[2];
	intersectionNormal = glm::normalize(intersectionNormal);
	if (depth < maxDepth) {
		depth += 1;
		glm::vec3 reflection = glm::normalize(rayDir - 2.0f * intersectionNormal * glm::dot(rayDir, intersectionNormal));
		colour = castRay(scene, intersection.intersectionPoint, reflection, lightPos, lightStrength, ambientLightStrength, triangle.objName, depth);
	}
	if (!inShadow){
		// calculate specular highlight
		float specPoint = specularMultiplier(lightPos, intersection.intersectionPoint, intersectionNormal, rayDir, 256);
		colour.red += (255.0f * specPoint);
		colour.green += (255.0f * specPoint);
		colour.blue += (255.0f * specPoint);
	}

	// mirrors don't always reflect 100% of light - here we pretend they only reflect 80%
	colour.red *= 0.8;
	colour.green *= 0.8;
	colour.blue *= 0.8;

	// Cap colour at (255, 255, 255)
	colour.red = std::min(colour.red, 255);
	colour.green = std::min(colour.green, 255);
	colour.blue = std::min(colour.blue, 255);
	return colour;
}

Colour castRay(std::vector<Model> &scene, glm::vec3 origin, glm::vec3 direction, glm::vec3 lightPos, float lightStrength, float ambientLightStrength, std::string originObjName, int depth) {
	float tSmallest = MAXFLOAT;
	RayTriangleIntersection intersection;

	for (Model& model : scene){
		for (int i = 0; i < model.triangles.size(); i++) {
			ModelTriangle &triangle = model.triangles.at(i);
			if (depth > 0 && triangle.objName == originObjName) continue;  // don't want to hit ourselves (for mirrors etc.)
			glm::vec3 &v0 = triangle.vertices[0];
			glm::vec3 &v1 = triangle.vertices[1];
			glm::vec3 &v2 = triangle.vertices[2];

			glm::vec3 e0 = v1 - v0;
			glm::vec3 e1 = v2 - v0;
			glm::vec3 tuv = glm::inverse(glm::mat3(-direction, e0, e1)) * (origin - v0);
			float t = tuv.x;
			float u = tuv.y;
			float v = tuv.z;
			if (t >= 0 && u > 0 && v > 0 && u + v < 1) {
				// we hit this triangle
				if (t < tSmallest) {
					tSmallest = t;
					intersection.intersectedTriangle = triangle;
					intersection.intersectionPoint = v0 + e0 * u + e1 * v;
					intersection.distanceFromCamera = t;
					intersection.u = u;
					intersection.v = v;
				}
			}
		}
	}
	if (tSmallest < MAXFLOAT) {
		// we found a hit in the above loop
		Colour &colour = intersection.intersectedTriangle.colour;
		int maxDepth = 15;
		bool inShadow = calculateShadows(scene, lightPos, intersection.intersectionPoint, intersection.intersectedTriangle.objName);
		if (inShadow) {
			switch (intersection.intersectedTriangle.type){
				case MIRROR:
					colour = mirror(scene, lightPos, lightStrength, ambientLightStrength, direction, intersection, depth, maxDepth, inShadow);
					break;
				case PHONG_MIRROR:
					colour = phongMirror(scene, lightPos, lightStrength, ambientLightStrength, direction, intersection, depth, maxDepth, inShadow);
					break;
				default:
					colour = ambientLightOnly(ambientLightStrength, intersection);
					break;
			}
		} else {
			switch (intersection.intersectedTriangle.type) {
				case FLAT:
					colour = diffuseAmbientShading(lightPos, lightStrength, ambientLightStrength, direction, intersection);
					break;
				case FLAT_SPECULAR:
					colour = diffuseAmbientShading(lightPos, lightStrength, ambientLightStrength, direction, intersection);
					colour = specularShading(lightPos, lightStrength, ambientLightStrength, direction, intersection);
					break;
				case SMOOTH_GOURAUD:
					colour = gouraudShading(lightPos, lightStrength, ambientLightStrength, direction, intersection);
					break;
				case SMOOTH_PHONG:
					colour = phongShading(lightPos, lightStrength, ambientLightStrength, direction, intersection);
					break;
				case MIRROR:
					colour = mirror(scene, lightPos, lightStrength, ambientLightStrength, direction, intersection, depth, maxDepth, inShadow);
					break;
				case PHONG_MIRROR:
					colour = phongMirror(scene, lightPos, lightStrength, ambientLightStrength, direction, intersection, depth, maxDepth, inShadow);
					break;
			}
		}
		return colour;
	}
	return {0, 0, 0};
}

void raytrace(DrawingWindow &window, std::vector<Model> &scene, glm::vec4 &camVec, glm::mat4 &camRot, glm::mat4 &modRot, float focalLength) {
	// Light Data
	glm::vec3 lightPos = {-0.3, 0.9, 0.8};
	float lightStrength = 10;
	float ambientLightStrength = 0.3;
	// Transform Light Position
	glm::vec4 lightPos4{lightPos, 1};
	lightPos4 = modRot * lightPos4;
	lightPos4 -= camVec;
	lightPos4 = camRot * lightPos4;
	lightPos = {lightPos4.x, lightPos4.y, lightPos4.z};

	// transform triangle vertices
	// note: copying here instead of using reference as we do not want to transform the actual triangles that are passed into this function
	// FIXME: WE DONT TRANSFORM NORMALS!!!!!!
	// std::vector<ModelTriangle> transformedTriangles;
	// for (ModelTriangle triangle : triangles) {
	// 	for (glm::vec3 &vertex : triangle.vertices) {
	// 		glm::vec4 vertex4{vertex, 1};
	// 		vertex4 = modRot * vertex4;
	// 		vertex4 -= camVec;
	// 		vertex4 = camRot * vertex4;
	// 		vertex = {vertex4.x, vertex4.y, vertex4.z};
	// 	}
	// 	transformedTriangles.push_back(triangle);
	// }

	glm::vec3 origin = {0,0,0};
	// for (int h = 0; h < window.height; h++) {
	// 	for (int w = 0; w < window.width; w++) {
	// 		// (de)scale by 100
	// 		float worldX = (w - window.width / 2.0f) / 100;
	// 		float worldY = -(h - window.height / 2.0f) / 100;
	// 		glm::vec3 rayDir = glm::normalize(glm::vec3{worldX, worldY, -focalLength} - origin);

	// 		Colour colour = castRay(transformedTriangles, origin, rayDir, lightPos, lightStrength, ambientLightStrength);
	// 		draw(window, w, h, colour);
	// 	}
	// }
}

void calculateFaceNormals(std::vector<ModelTriangle> &triangles) {
	for (ModelTriangle &triangle : triangles) {
		triangle.normal = glm::normalize(glm::cross(triangle.vertices[1] - triangle.vertices[0], triangle.vertices[2] - triangle.vertices[0]));
	}
}

void calculateVertexNormals(std::vector<ModelTriangle> &triangles) {
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

void setNameTypeAndShadows(std::vector<ModelTriangle> &triangles, std::string name, TriangleType type, bool shadows) {
	for (ModelTriangle &triangle : triangles) {
		triangle.objName = name;
		triangle.type = type;
		triangle.shadows = shadows;
	}
}

void loadModel(std::vector<Model>& scene, std::string objFile, std::string mtlFile, float scale, std::string name, TriangleType type, bool shadows){
	Model model{readObjFile(objFile, mtlFile, scale), name};
	setNameTypeAndShadows(model.triangles, name, type, shadows);
	if (type == PHONG_MIRROR || type == SMOOTH_GOURAUD || type == SMOOTH_PHONG)
		calculateVertexNormals(model.triangles);
	else
		calculateFaceNormals(model.triangles);
	scene.emplace_back(model);
}

int main(int argc, char *argv[]) {
	bool rasterising = true;
	bool raytracedOnce = false;
	float focalLength = 4;
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;

	// load models
	std::vector<Model> scene;
	loadModel(scene, "red-box.obj", "cornell-box.mtl", 0.35, "redBox", FLAT_SPECULAR, true);
	loadModel(scene, "blue-box.obj", "cornell-box.mtl", 0.35, "blueBox", MIRROR, true);
	loadModel(scene, "left-wall.obj", "cornell-box.mtl", 0.35, "leftWall", FLAT_SPECULAR, true);
	loadModel(scene, "right-wall.obj", "cornell-box.mtl", 0.35, "rightWall", FLAT_SPECULAR, true);
	loadModel(scene, "back-wall.obj", "cornell-box.mtl", 0.35, "backWall", FLAT_SPECULAR, true);
	loadModel(scene, "ceiling.obj", "cornell-box.mtl", 0.35, "ceiling", FLAT_SPECULAR, true);
	loadModel(scene, "floor.obj", "cornell-box.mtl", 0.35, "floor", FLAT_SPECULAR, true);
	loadModel(scene, "sphere.obj", "", 0.35, "sphere", SMOOTH_PHONG, true);

	glm::vec4 camVec = Translate(0, 0, 3) * glm::vec4(0, 0, 0, 1);
	glm::mat4 camRot{1};
	glm::mat4 modRot{1};
	modRot = Rotate(0, 4, 0) * modRot;


	Camera camera {focalLength};
	camera.translate(0, 0, 3);
	// camera.rotate(10, 0, 0);

	while (true) {
		if (rasterising) {
			window.clearPixels();
			rasterise(window, scene, camera);
		} else if (!raytracedOnce) {
			window.clearPixels();
			std::cout << "starting raytracing\n";
			raytrace(window, scene, camVec, camRot, modRot, focalLength);
			raytracedOnce = true;
			std::cout << "finished raytracing\n";
		}
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window, camera, rasterising, raytracedOnce);
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}
