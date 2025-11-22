#include <CanvasPoint.h>
#include <CanvasTriangle.h>
#include <Colour.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <map>
#include "Draw.hpp"
#include "ObjReader.hpp"
#include "RayTriangleIntersection.h"
#include "Transform.hpp"

// vec3 comparator so I can use as key in map
struct vec3Compare {
	bool operator()(const glm::vec3 &a, const glm::vec3 &b) const {
		if (a.x != b.x) return a.x < b.x;
		if (a.y != b.y) return a.y < b.y;
		return a.z < b.z;
	}
};

CanvasPoint getCanvasIntersectionPoint(glm::vec4 &cameraPosition, glm::mat4 &cameraRotation, glm::mat4 &modelRotation, glm::vec3 &vertexPosition, float focalLength) {
	glm::vec4 vertexPos{vertexPosition, 1};

	vertexPos = modelRotation * vertexPos;
	vertexPos -= cameraPosition;
	vertexPos = cameraRotation * vertexPos;

	// FIXME: ask TA's about scaling model and funny behaviour when scale = 100, also about black lines appearing when rotating etc
	// also about boxes going through floor - is it ok??
	vertexPos.x *= 100;
	// same reason as below for negative sign
	vertexPos.y *= -100;
	// blue box has smaller z coord than red box, so need to flip (works because coords are relative to origin (middle) of object)
	vertexPos.z *= -1;
	return {(vertexPos.x * focalLength) / vertexPos.z + WIDTH / 2.0f, (vertexPos.y * focalLength) / vertexPos.z + HEIGHT / 2.0f, vertexPos.z};
}

CanvasTriangle getCanvasIntersectionTriangle(glm::vec4 &cameraPosition, glm::mat4 &cameraRotation, glm::mat4 &modelRotation, ModelTriangle &triangle, float focalLength) {
	CanvasPoint v0 = getCanvasIntersectionPoint(cameraPosition, cameraRotation, modelRotation, triangle.vertices[0], focalLength);
	CanvasPoint v1 = getCanvasIntersectionPoint(cameraPosition, cameraRotation, modelRotation, triangle.vertices[1], focalLength);
	CanvasPoint v2 = getCanvasIntersectionPoint(cameraPosition, cameraRotation, modelRotation, triangle.vertices[2], focalLength);
	return {v0, v1, v2};
}

void handleEvent(SDL_Event event, DrawingWindow &window, glm::vec4 &camVec, glm::mat4 &camRot, glm::mat4 &modRot, bool &rasterising, bool &raytracedOnce) {
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_LEFT)
			camVec = Translate(0.25, 0, 0) * camVec;
		if (event.key.keysym.sym == SDLK_RIGHT)
			camVec = Translate(-0.25, 0, 0) * camVec;
		if (event.key.keysym.sym == SDLK_UP)
			camVec = Translate(0, -0.25, 0) * camVec;
		if (event.key.keysym.sym == SDLK_DOWN)
			camVec = Translate(0, 0.25, 0) * camVec;
		if (event.key.keysym.sym == SDLK_f)
			camVec = Translate(0, 0, -0.25) * camVec;
		if (event.key.keysym.sym == SDLK_b)
			camVec = Translate(0, 0, 0.25) * camVec;
		// cam rot
		if (event.key.keysym.sym == SDLK_w)
			camRot = Rotate(1, 0, 0) * camRot;
		if (event.key.keysym.sym == SDLK_s)
			camRot = Rotate(-1, 0, 0) * camRot;
		if (event.key.keysym.sym == SDLK_a)
			camRot = Rotate(0, -1, 0) * camRot;
		if (event.key.keysym.sym == SDLK_d)
			camRot = Rotate(0, 1, 0) * camRot;
		if (event.key.keysym.sym == SDLK_o)
			camRot = Rotate(0, 0, 1) * camRot;
		if (event.key.keysym.sym == SDLK_p)
			camRot = Rotate(0, 0, -1) * camRot;
		// mod rot
		if (event.key.keysym.sym == SDLK_l)
			modRot = Rotate(0, -2, 0) * modRot;
		if (event.key.keysym.sym == SDLK_h)
			modRot = Rotate(0, 2, 0) * modRot;
		if (event.key.keysym.sym == SDLK_k)
			modRot = Rotate(2, 0, 0) * modRot;
		if (event.key.keysym.sym == SDLK_j)
			modRot = Rotate(-2, 0, 0) * modRot;
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

void rasterise(DrawingWindow &window, std::vector<ModelTriangle> &triangles, glm::vec4 &camVec, glm::mat4 &camRot, glm::mat4 &modRot, float focalLength) {
	// reset depth buffer
	std::vector<float> depthBuffer((window.width * window.height), 0);
	for (ModelTriangle &triangle : triangles) {
		bool draw = true;
		CanvasTriangle canvasTri = getCanvasIntersectionTriangle(camVec, camRot, modRot, triangle, focalLength);
		for (CanvasPoint &vertexPos : canvasTri.vertices)
			if (vertexPos.x >= WIDTH || vertexPos.x < 0 || vertexPos.y >= HEIGHT || vertexPos.y < 0) draw = false;
		if (draw) drawFilledTriangle(window, depthBuffer, canvasTri, triangle.colour);
	}
}

bool calculateShadows(std::vector<ModelTriangle> triangles, glm::vec3 lightPos, glm::vec3 intersectionPoint) {
	glm::vec3 shadowRayDir = lightPos - intersectionPoint;
	float distToLight = glm::length(shadowRayDir);
	// we normalise after calculating distToLight so that distToLight is actual distance,
	// but also so that t == distToLight (as the vector which t scales - shadowRayDir - has length 1 after normalisation)
	shadowRayDir = glm::normalize(shadowRayDir);

	bool needShadow = false;

	for (ModelTriangle &triangle : triangles) {
		glm::vec3 &v0 = triangle.vertices[0];
		glm::vec3 &v1 = triangle.vertices[1];
		glm::vec3 &v2 = triangle.vertices[2];

		glm::vec3 e0 = v1 - v0;
		glm::vec3 e1 = v2 - v0;
		glm::vec3 tuv = glm::inverse(glm::mat3(-shadowRayDir, e0, e1)) * (intersectionPoint - v0);
		float t = tuv.x;
		float u = tuv.y;
		float v = tuv.z;
		// t > 0.00001 prevents shadow achne
		if (t >= 0.001 && u > 0 && v > 0 && u + v < 1) {
			// we hit a triangle
			if (t < distToLight) {
				needShadow = true;
				break;
			}
		}
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

Colour flatShading(glm::vec3 lightPos, float lightStrength, float ambientLightStrength, glm::vec3 rayDir, RayTriangleIntersection &intersection) {
	float specular = 0;

	Colour colour = intersection.intersectedTriangle.colour;
	glm::vec3 normal = intersection.intersectedTriangle.normal;
	glm::vec3 intersectionPoint = intersection.intersectionPoint;

	// Calculate specular intensity
	glm::vec3 directionTriToLight = glm::normalize(lightPos - intersectionPoint);
	glm::vec3 reflection = directionTriToLight - 2.0f * normal * glm::dot(directionTriToLight, normal);
	float specAngle = std::max(glm::dot(reflection, rayDir), 0.0f);
	specular = std::pow(specAngle, 256);

	// Add diffuse and ambient lighting
	float light = diffuseAmbientMultiplier(lightPos, lightStrength, ambientLightStrength, intersectionPoint, normal);
	colour.red *= light;
	colour.green *= light;
	colour.blue *= light;

	// Add specular, assumes the light source is white (255, 255, 255)
	colour.red += (255.0f * specular);
	colour.green += (255.0f * specular);
	colour.blue += (255.0f * specular);

	// Cap colour at (255, 255, 255)
	colour.red = std::min(colour.red, 255);
	colour.green = std::min(colour.green, 255);
	colour.blue = std::min(colour.blue, 255);
	return colour;
}

Colour gouraudShading(glm::vec3 lightPos, float lightStrength, float ambientLightStrength, glm::vec3 rayDir, RayTriangleIntersection &intersection, float u, float v) {
	ModelTriangle triangle = intersection.intersectedTriangle;
	Colour colour = triangle.colour;
	float lightv0 = diffuseAmbientMultiplier(lightPos, lightStrength, ambientLightStrength, triangle.vertices[0], triangle.vertexNormals[0]);
	float lightv1 = diffuseAmbientMultiplier(lightPos, lightStrength, ambientLightStrength, triangle.vertices[1], triangle.vertexNormals[1]);
	float lightv2 = diffuseAmbientMultiplier(lightPos, lightStrength, ambientLightStrength, triangle.vertices[2], triangle.vertexNormals[2]);
	float lightPoint = (1 - u - v) * lightv0 + u * lightv1 + v * lightv2;
	// glm::vec3 intersectionNormal = (1 - u - v) * triangle.vertexNormals[0] + u * triangle.vertexNormals[1] + v * triangle.vertexNormals[2];
	// float lightPoint = diffuseAmbientMultiplier(lightPos, lightStrength, ambientLightStrength, intersection.intersectionPoint, intersectionNormal);
	colour.red *= lightPoint;
	colour.green *= lightPoint;
	colour.blue *= lightPoint;

	// Cap colour at (255, 255, 255)
	colour.red = std::min(colour.red, 255);
	colour.green = std::min(colour.green, 255);
	colour.blue = std::min(colour.blue, 255);
	return colour;
}

void raytrace(DrawingWindow &window, std::vector<ModelTriangle> &triangles, glm::vec4 &camVec, glm::mat4 &camRot, glm::mat4 &modRot, float focalLength) {
	// -------Light Data--------
	glm::vec3 lightPos = {0, 0, 2};
	float lightStrength = 10;
	float ambientLightStrength = 0.3;
	// Transform Light Position
	glm::vec4 lightPos4{lightPos, 1};
	lightPos4 = modRot * lightPos4;
	lightPos4 -= camVec;
	lightPos4 = camRot * lightPos4;
	lightPos = {lightPos4.x, lightPos4.y, lightPos4.z};

	float closestU, closestV;

	// transform triangle vertices
	// note: copying here instead of using reference as we do not want to transform the actual triangles that are passed into this function
	std::vector<ModelTriangle> transformedTriangles;
	for (ModelTriangle triangle : triangles) {
		for (glm::vec3 &vertex : triangle.vertices) {
			glm::vec4 vertex4{vertex, 1};
			vertex4 = modRot * vertex4;
			vertex4 -= camVec;
			vertex4 = camRot * vertex4;
			vertex = {vertex4.x, vertex4.y, vertex4.z};
		}
		transformedTriangles.push_back(triangle);
	}

	glm::vec3 camVeccy = {camVec.x, camVec.y, camVec.z};

	for (int h = 0; h < window.height; h++) {
		for (int w = 0; w < window.width; w++) {
			// (de)scale by 100
			float worldX = (w - window.width / 2.0f) / 100;
			float worldY = -(h - window.height / 2.0f) / 100;
			glm::vec3 rayDir = glm::normalize(glm::vec3{worldX, worldY, -focalLength} - camVeccy);

			float tSmallest = MAXFLOAT;
			RayTriangleIntersection intersection;

			for (ModelTriangle &triangle : transformedTriangles) {
				glm::vec3 &v0 = triangle.vertices[0];
				glm::vec3 &v1 = triangle.vertices[1];
				glm::vec3 &v2 = triangle.vertices[2];

				glm::vec3 e0 = v1 - v0;
				glm::vec3 e1 = v2 - v0;
				glm::vec3 tuv = glm::inverse(glm::mat3(-rayDir, e0, e1)) * (camVeccy - v0);
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
						closestU = u;
						closestV = v;
					}
				}
			}
			if (tSmallest < MAXFLOAT) {
				// we found a hit in the above loop
				Colour colour = intersection.intersectedTriangle.colour;
				// if (!calculateShadows(transformedTriangles, lightPos, intersection.intersectionPoint)) {
				colour = gouraudShading(lightPos, lightStrength, ambientLightStrength, rayDir, intersection, closestU, closestV);
				// colour = flatShading(lightPos, lightStrength, ambientLightStrength, rayDir, intersection);
				// } else {
				// 	colour = ambientLightOnly(ambientLightStrength, intersection);
				// }
				draw(window, w, h, colour);
			}
		}
	}
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
		// std::cout << glm::to_string(triangle.vertexNormals[0]) << "  ";
	}
}

int main(int argc, char *argv[]) {
	bool rasterising = false;
	bool raytracedOnce = false;
	float focalLength = 3;
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;

	std::vector<ModelTriangle> cornell;
	// load cornell box model and calculate normals
	// cornell = readObjFile("cornell-box.obj", "cornell-box.mtl", 0.35);
	// calculateFaceNormals(cornell);

	// load sphere model and calculate normals
	std::vector<ModelTriangle> sphere = readObjFile("sphere.obj", "", 0.35);
	calculateVertexNormals(sphere);
	// append sphere's triangles to cornell's
	cornell.insert(cornell.end(), std::make_move_iterator(sphere.begin()), std::make_move_iterator(sphere.end()));

	glm::vec4 camVec = Translate(0, 0, 2) * glm::vec4(0, 0, 0, 1);
	glm::mat4 camRot{1};
	glm::mat4 modRot{1};

	while (true) {
		if (rasterising) {
			window.clearPixels();
			rasterise(window, cornell, camVec, camRot, modRot, focalLength);
		} else if (!raytracedOnce) {
			window.clearPixels();
			raytrace(window, cornell, camVec, camRot, modRot, focalLength);
			raytracedOnce = true;
		}
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window, camVec, camRot, modRot, rasterising, raytracedOnce);
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}
