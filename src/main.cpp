#include <CanvasPoint.h>
#include <CanvasTriangle.h>
#include <Colour.h>
#include <DrawingWindow.h>
#include <ModelTriangle.h>
#include <Utils.h>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iomanip>
#include <map>
#include <sstream>
#include "Camera.hpp"
#include "Draw.hpp"
#include "Light.hpp"
#include "Model.hpp"
#include "RayTriangleIntersection.h"
#include "animate.hpp"

Colour castRay(std::map<std::string, Model> &scene, glm::vec3 origin, glm::vec3 direction, Light &light, int depth = 0, std::string originObjName = "", ModelTriangle *tri = nullptr);

void handleEvent(SDL_Event event, DrawingWindow &window, Camera &camera) {
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
		if (event.key.keysym.sym == SDLK_l) {
			camera.rotatePosition(0, -2, 0);
			camera.lookat(0, 0, 0);
		}
		if (event.key.keysym.sym == SDLK_h) {
			camera.rotatePosition(0, 2, 0);
			camera.lookat(0, 0, 0);
		}
		if (event.key.keysym.sym == SDLK_k) {
			camera.rotatePosition(2, 0, 0);
			camera.lookat(0, 0, 0);
		}
		if (event.key.keysym.sym == SDLK_j) {
			camera.rotatePosition(-2, 0, 0);
			camera.lookat(0, 0, 0);
		}
	} else if (event.type == SDL_MOUSEBUTTONDOWN) {
		window.savePPM("output.ppm");
		window.saveBMP("output.bmp");
	}
}

CanvasPoint getCanvasIntersectionPoint(Camera &camera, glm::vec3 vertexPosition) {
	// updadte vertex position based on camera transformations
	vertexPosition = (vertexPosition - camera.position) * camera.orientation;

	vertexPosition.x *= 100;
	// same reason as below for negative sign
	vertexPosition.y *= -100;
	// blue box has smaller z coord than red box, so need to flip (works because coords are relative to origin (middle) of object)
	vertexPosition.z *= -1;
	return {(vertexPosition.x * camera.focalLength) / vertexPosition.z + WIDTH / 2.0f, (vertexPosition.y * camera.focalLength) / vertexPosition.z + HEIGHT / 2.0f, vertexPosition.z};
}

CanvasTriangle getCanvasIntersectionTriangle(Camera &camera, ModelTriangle &triangle) {
	CanvasPoint v0 = getCanvasIntersectionPoint(camera, triangle.vertices[0]);
	CanvasPoint v1 = getCanvasIntersectionPoint(camera, triangle.vertices[1]);
	CanvasPoint v2 = getCanvasIntersectionPoint(camera, triangle.vertices[2]);
	return {v0, v1, v2};
}

void rasterise(DrawingWindow &window, std::map<std::string, Model> &scene, Camera &camera) {
	// reset depth buffer
	std::vector<float> depthBuffer((window.width * window.height), 0);
	for (auto &pair : scene) {
		Model &model = pair.second;
		for (ModelTriangle &triangle : model.triangles) {
			bool draw = true;
			// transform camera????
			CanvasTriangle canvasTri = getCanvasIntersectionTriangle(camera, triangle);
			if (draw && model.name == "backWall" && !model.shadows) {
				canvasTri.v0().texturePoint = {195, 5};
				canvasTri.v1().texturePoint = {395, 380};
				canvasTri.v2().texturePoint = {65, 330};
				drawTexturedTriangle(window, canvasTri, depthBuffer, {"texture.ppm"});
			} else if (draw)
				drawFilledTriangle(window, depthBuffer, canvasTri, triangle.colour);
		}
	}
}

void wireframe(DrawingWindow &window, std::map<std::string, Model> &scene, Camera &camera) {
	for (auto &pair : scene) {
		Model &model = pair.second;
		for (ModelTriangle &triangle : model.triangles) {
			bool draw = true;
			// transform camera????
			CanvasTriangle canvasTri = getCanvasIntersectionTriangle(camera, triangle);
			// dodgy culling
			for (CanvasPoint &vertexPos : canvasTri.vertices)
				if (vertexPos.x >= WIDTH || vertexPos.x < 0 || vertexPos.y >= HEIGHT || vertexPos.y < 0) draw = false;
			if (draw) drawStokedTriangle(window, canvasTri, triangle.colour);
		}
	}
}

float calculateShadows(std::map<std::string, Model> &scene, glm::vec3 lightPosition, glm::vec3 intersectionPoint, std::string originObjName) {
	glm::vec3 shadowRayDir = lightPosition - intersectionPoint;
	float distToLight = glm::length(shadowRayDir);
	// we normalise after calculating distToLight so that distToLight is actual distance,
	// but also so that t == distToLight (as the vector which t scales - shadowRayDir - has length 1 after normalisation)
	shadowRayDir = glm::normalize(shadowRayDir);

	float lightIntensity = 1;
	float minT = distToLight;

	for (auto &pair : scene) {
		Model &model = pair.second;
		if (model.name == originObjName) continue;	// don't want to hit ourselves
		for (ModelTriangle &triangle : model.triangles) {
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
					if (model.shadows) lightIntensity = 0;
				}
			}
		}
	}
	return lightIntensity;
}

float diffuseMultiplier(Light &light, glm::vec3 intersectionPoint, glm::vec3 normal) {
	float diffuse = 0;
	if (light.type == POINT) {
		float distanceTriToLight = glm::length(light.position - intersectionPoint);
		glm::vec3 directionTriToLight = glm::normalize(light.position - intersectionPoint);
		float dotProd = std::max(glm::dot(directionTriToLight, normal), 0.0f);
		diffuse = (light.strength * dotProd) / (4 * M_PI * distanceTriToLight);
	} else if (light.type == AREA) {
		for (int u = 0; u < light.uSize; u++) {
			for (int v = 0; v < light.vSize; v++) {
				glm::vec3 lightPosition = light.position + light.uStep * (u + std::rand() / float(RAND_MAX)) + light.vStep * (v + std::rand() / float(RAND_MAX));
				float distanceTriToLight = glm::length(lightPosition - intersectionPoint);
				glm::vec3 directionTriToLight = glm::normalize(lightPosition - intersectionPoint);
				float dotProd = std::max(glm::dot(directionTriToLight, normal), 0.0f);
				diffuse += (light.strength * dotProd) / (4 * M_PI * distanceTriToLight);
			}
		}
		diffuse /= (light.uSize * light.vSize);
	}

	return diffuse;
}

Colour diffuseAmbientShading(Light &light, glm::vec3 rayDir, RayTriangleIntersection &intersection, float lightIntensity) {
	Colour colour = intersection.intersectedTriangle.colour;
	glm::vec3 normal = intersection.intersectedTriangle.normal;
	glm::vec3 intersectionPoint = intersection.intersectionPoint;

	float diffuse = diffuseMultiplier(light, intersectionPoint, normal) * lightIntensity;
	float diffuseAmbient = diffuse + light.ambientStrength;
	colour.red *= diffuseAmbient;
	colour.green *= diffuseAmbient;
	colour.blue *= diffuseAmbient;

	// Cap colour at (255, 255, 255)
	colour.red = std::min(colour.red, 255);
	colour.green = std::min(colour.green, 255);
	colour.blue = std::min(colour.blue, 255);
	return colour;
}

float specularMultiplier(Light &light, glm::vec3 intersectionPoint, glm::vec3 normal, glm::vec3 rayDir, int specExponent) {
	float specular = 0;
	if (light.type == POINT) {
		glm::vec3 directionTriToLight = glm::normalize(light.position - intersectionPoint);
		glm::vec3 reflection = directionTriToLight - 2.0f * normal * glm::dot(directionTriToLight, normal);
		float specAngle = std::max(glm::dot(reflection, rayDir), 0.0f);
		specular = std::pow(specAngle, specExponent);
	} else if (light.type == AREA) {
		for (int u = 0; u < light.uSize; u++) {
			for (int v = 0; v < light.vSize; v++) {
				glm::vec3 lightPosition = light.position + light.uStep * (u + std::rand() / float(RAND_MAX)) + light.vStep * (v + std::rand() / float(RAND_MAX));
				glm::vec3 directionTriToLight = glm::normalize(lightPosition - intersectionPoint);
				glm::vec3 reflection = directionTriToLight - 2.0f * normal * glm::dot(directionTriToLight, normal);
				float specAngle = std::max(glm::dot(reflection, rayDir), 0.0f);
				specular += std::pow(specAngle, specExponent);
			}
		}
		specular /= (light.uSize * light.vSize);
	}
	return specular;
}

Colour specularShading(Light &light, glm::vec3 rayDir, RayTriangleIntersection &intersection, float lightIntensity) {
	Colour colour = intersection.intersectedTriangle.colour;
	glm::vec3 normal = intersection.intersectedTriangle.normal;
	glm::vec3 intersectionPoint = intersection.intersectionPoint;

	// Add specular, assumes the light source is white (255, 255, 255)
	float specular = specularMultiplier(light, intersectionPoint, normal, rayDir, 256) * lightIntensity;
	colour.red += (255.0f * specular);
	colour.green += (255.0f * specular);
	colour.blue += (255.0f * specular);

	// Cap colour at (255, 255, 255)
	colour.red = std::min(colour.red, 255);
	colour.green = std::min(colour.green, 255);
	colour.blue = std::min(colour.blue, 255);
	return colour;
}

Colour gouraudShading(Light &light, glm::vec3 rayDir, RayTriangleIntersection &intersection, float lightIntensity) {
	ModelTriangle triangle = intersection.intersectedTriangle;
	Colour colour = triangle.colour;
	float u = intersection.u;
	float v = intersection.v;
	float lightv0 = diffuseMultiplier(light, triangle.vertices[0], triangle.vertexNormals[0]) * lightIntensity + light.ambientStrength;
	float lightv1 = diffuseMultiplier(light, triangle.vertices[1], triangle.vertexNormals[1]) * lightIntensity + light.ambientStrength;
	float lightv2 = diffuseMultiplier(light, triangle.vertices[2], triangle.vertexNormals[2]) * lightIntensity + light.ambientStrength;
	float lightPoint = (1 - u - v) * lightv0 + u * lightv1 + v * lightv2;

	float specv0 = specularMultiplier(light, triangle.vertices[0], triangle.vertexNormals[0], rayDir, 16) * lightIntensity;
	float specv1 = specularMultiplier(light, triangle.vertices[1], triangle.vertexNormals[1], rayDir, 16) * lightIntensity;
	float specv2 = specularMultiplier(light, triangle.vertices[2], triangle.vertexNormals[2], rayDir, 16) * lightIntensity;
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

Colour phongShading(Light &light, glm::vec3 rayDir, RayTriangleIntersection &intersection, float lightIntensity) {
	ModelTriangle triangle = intersection.intersectedTriangle;
	Colour colour = triangle.colour;
	float u = intersection.u;
	float v = intersection.v;
	glm::vec3 intersectionNormal = (1 - u - v) * triangle.vertexNormals[0] + u * triangle.vertexNormals[1] + v * triangle.vertexNormals[2];
	intersectionNormal = glm::normalize(intersectionNormal);
	float lightPoint = diffuseMultiplier(light, intersection.intersectionPoint, intersectionNormal) * lightIntensity + light.ambientStrength;
	float specPoint = specularMultiplier(light, intersection.intersectionPoint, intersectionNormal, rayDir, 16) * lightIntensity;
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

Colour mirror(std::map<std::string, Model> &scene, Light &light, glm::vec3 rayDir, RayTriangleIntersection &intersection, int depth, int maxDepth, float lightIntensity) {
	Colour colour;
	if (depth < maxDepth) {
		depth += 1;
		glm::vec3 reflection = glm::normalize(rayDir - 2.0f * intersection.intersectedTriangle.normal * glm::dot(rayDir, intersection.intersectedTriangle.normal));
		colour = castRay(scene, intersection.intersectionPoint, reflection, light, depth, intersection.intersectedModel.name);
	}
	// calculate specular highlight
	float specular = specularMultiplier(light, intersection.intersectionPoint, intersection.intersectedTriangle.normal, rayDir, 512) * lightIntensity;
	colour.red += (255.0f * specular);
	colour.green += (255.0f * specular);
	colour.blue += (255.0f * specular);
	// TODO: need this?????
	// mirrors don't always reflect 100% of light - here we pretend they only reflect up to 80%
	colour.red *= std::min(lightIntensity + light.ambientStrength, 0.8f);
	colour.green *= std::min(lightIntensity + light.ambientStrength, 0.8f);
	colour.blue *= std::min(lightIntensity + light.ambientStrength, 0.8f);

	// Cap colour at (255, 255, 255)
	colour.red = std::min(colour.red, 255);
	colour.green = std::min(colour.green, 255);
	colour.blue = std::min(colour.blue, 255);
	return colour;
}

Colour mirrorPhong(std::map<std::string, Model> &scene, Light &light, glm::vec3 rayDir, RayTriangleIntersection &intersection, int depth, int maxDepth, float lightIntensity) {
	Colour colour;
	float u = intersection.u;
	float v = intersection.v;
	ModelTriangle &triangle = intersection.intersectedTriangle;
	glm::vec3 intersectionNormal = (1 - u - v) * triangle.vertexNormals[0] + u * triangle.vertexNormals[1] + v * triangle.vertexNormals[2];
	intersectionNormal = glm::normalize(intersectionNormal);
	if (depth < maxDepth) {
		depth += 1;
		glm::vec3 reflection = glm::normalize(rayDir - 2.0f * intersectionNormal * glm::dot(rayDir, intersectionNormal));
		colour = castRay(scene, intersection.intersectionPoint, reflection, light, depth, intersection.intersectedModel.name);
	}
	// calculate specular highlight
	float specular = specularMultiplier(light, intersection.intersectionPoint, intersection.intersectedTriangle.normal, rayDir, 512) * lightIntensity;
	colour.red += (255.0f * specular);
	colour.green += (255.0f * specular);
	colour.blue += (255.0f * specular);
	// mirrors don't always reflect 100% of light - here we pretend they only reflect up to 80%
	colour.red *= std::min(lightIntensity + light.ambientStrength, 0.8f);
	colour.green *= std::min(lightIntensity + light.ambientStrength, 0.8f);
	colour.blue *= std::min(lightIntensity + light.ambientStrength, 0.8f);

	// Cap colour at (255, 255, 255)
	colour.red = std::min(colour.red, 255);
	colour.green = std::min(colour.green, 255);
	colour.blue = std::min(colour.blue, 255);
	return colour;
}

glm::vec3 refractRay(glm::vec3 rayDir, glm::vec3 intersectionNormal, float rIndexi, float rIndext, float cosi, float cost) {
	float relativeRIndex = rIndexi / rIndext;
	if (cost < 0)
		// total internal reflection. There is no refraction in this case
		return glm::vec3{0};
	else
		return relativeRIndex * rayDir + (relativeRIndex * cosi - std::sqrtf(cost)) * intersectionNormal;
}

float fresnel(float rIndexi, float rIndext, float cosi, float cost) {
	if (cost < 0) {
		// total internal reflection
		return 1;
	} else {
		float rParallel = ((rIndext * cosi) - (rIndexi * cost)) / ((rIndext * cosi) + (rIndexi * cost));
		float rPerpendicular = ((rIndexi * cosi) - (rIndext * cost)) / ((rIndexi * cosi) + (rIndext * cost));
		return (std::pow(rParallel, 2) + std::pow(rPerpendicular, 2)) / 2;
	}
}

Colour transparentShading(std::map<std::string, Model> &scene, Light &light, glm::vec3 rayDir, RayTriangleIntersection &intersection, float refractionIndex, int depth, int maxDepth, float lightIntensity) {
	const float BIAS = 0.05;
	glm::vec3 normal = intersection.intersectedTriangle.normal;

	// calculate refractive index and cos for incident and transmitted ray
	float cosi = std::min(-1.0f, std::max(1.0f, glm::dot(normal, rayDir)));	 // clamp the dot product at (-1, 1)
	float rIndexi = 1;
	float rIndext = refractionIndex;
	if (cosi < 0) {
		cosi = -cosi;  // cos(theta_i) must be +ve
	} else {
		std::swap(rIndexi, rIndext);
	}
	float relativeRIndex = rIndexi / rIndext;
	float cost = 1 - std::pow(relativeRIndex, 2) * (1 - cosi * cosi);

	// compute fresnel
	float kr = fresnel(rIndexi, rIndext, cosi, cost);
	bool outside = glm::dot(normal, rayDir) < 0;
	glm::vec3 biasVec = BIAS * normal;
	// compute refraction if it is not a case of total internal reflection
	if (depth < maxDepth) {
		depth += 1;
		Colour refractionColour{0, 0, 0};
		if (kr < 1) {
			glm::vec3 refractionDir = glm::normalize(refractRay(rayDir, normal, rIndexi, rIndext, cosi, cost));
			glm::vec3 refractionRayOrig = outside ? intersection.intersectionPoint - biasVec : intersection.intersectionPoint + biasVec;
			refractionColour = castRay(scene, refractionRayOrig, refractionDir, light, depth, "", &(intersection.intersectedTriangle));
		}

		glm::vec3 reflectionDir = glm::normalize(rayDir - 2.0f * normal * glm::dot(rayDir, normal));
		glm::vec3 reflectionRayOrig = outside ? intersection.intersectionPoint + biasVec : intersection.intersectionPoint - biasVec;
		Colour reflectionColour = castRay(scene, reflectionRayOrig, reflectionDir, light, depth, "", &intersection.intersectedTriangle);

		// mix the two
		Colour colour;
		colour.red = reflectionColour.red * kr + refractionColour.red * (1 - kr);
		colour.green = reflectionColour.green * kr + refractionColour.green * (1 - kr);
		colour.blue = reflectionColour.blue * kr + refractionColour.blue * (1 - kr);

		// calculate specular highlight
		float specPoint = specularMultiplier(light, intersection.intersectionPoint, normal, rayDir, 256) * lightIntensity;
		colour.red += (255.0f * specPoint);
		colour.green += (255.0f * specPoint);
		colour.blue += (255.0f * specPoint);
		// glass doesn't always transmit/reflect 100% of light - here we pretend it's only 98%
		// TODO: what should I do with this??? looks kinda cool - like it's slightly grey glass
		colour.red *= 0.98;
		colour.green *= 0.98;
		colour.blue *= 0.98;

		// Cap colour at (255, 255, 255)
		colour.red = std::min(colour.red, 255);
		colour.green = std::min(colour.green, 255);
		colour.blue = std::min(colour.blue, 255);
		return colour;
	}
	return {0, 0, 0};
}

Colour transparentShadingPhong(std::map<std::string, Model> &scene, Light &light, glm::vec3 rayDir, RayTriangleIntersection &intersection, float refractionIndex, int depth, int maxDepth, float lightIntensity) {
	const float BIAS = 0.05;
	Colour refractionColour{0, 0, 0};

	float u = intersection.u;
	float v = intersection.v;
	ModelTriangle &triangle = intersection.intersectedTriangle;
	glm::vec3 normal = (1 - u - v) * triangle.vertexNormals[0] + u * triangle.vertexNormals[1] + v * triangle.vertexNormals[2];
	normal = glm::normalize(normal);

	// calculate refractive index and cos for incident and transmitted ray
	float cosi = std::min(-1.0f, std::max(1.0f, glm::dot(normal, rayDir)));	 // clamp the dot product at (-1, 1)
	float rIndexi, rIndext;
	if (cosi < 0) {
		cosi = -cosi;  // cos(theta_i) must be +ve
		rIndexi = 1;
		rIndext = refractionIndex;
	} else {
		rIndexi = refractionIndex;
		rIndext = 1;
	}
	float relativeRIndex = rIndexi / rIndext;
	float cost = 1 - std::pow(relativeRIndex, 2) * (1 - cosi * cosi);

	// compute fresnel
	float kr = fresnel(rIndexi, rIndext, cosi, cost);
	// int kr = 0;
	bool outside = glm::dot(normal, rayDir) < 0;
	glm::vec3 biasVec = BIAS * normal;
	// compute refraction if it is not a case of total internal reflection
	if (depth < maxDepth) {
		depth += 1;
		if (kr < 1) {
			glm::vec3 refractionDir = glm::normalize(refractRay(rayDir, normal, rIndexi, rIndext, cosi, cost));
			glm::vec3 refractionRayOrig = outside ? intersection.intersectionPoint - biasVec : intersection.intersectionPoint + biasVec;
			refractionColour = castRay(scene, refractionRayOrig, refractionDir, light, depth, "", &triangle);
		}

		glm::vec3 reflectionDir = glm::normalize(rayDir - 2.0f * normal * glm::dot(rayDir, normal));
		glm::vec3 reflectionRayOrig = outside ? intersection.intersectionPoint + biasVec : intersection.intersectionPoint - biasVec;
		Colour reflectionColour = castRay(scene, reflectionRayOrig, reflectionDir, light, depth, "", &triangle);

		// mix the two
		Colour colour;
		colour.red = reflectionColour.red * kr + refractionColour.red * (1 - kr);
		colour.green = reflectionColour.green * kr + refractionColour.green * (1 - kr);
		colour.blue = reflectionColour.blue * kr + refractionColour.blue * (1 - kr);

		// calculate specular highlight
		float specPoint = specularMultiplier(light, intersection.intersectionPoint, normal, rayDir, 256) * lightIntensity;
		colour.red += (255.0f * specPoint);
		colour.green += (255.0f * specPoint);
		colour.blue += (255.0f * specPoint);
		// glass doesn't always transmit/reflect 100% of light - here we pretend it's only 98%
		// TODO: what should I do with this??? looks kinda cool - like it's slightly grey glass
		// colour.red *= 0.98;
		// colour.green *= 0.98;
		// colour.blue *= 0.98;

		// Cap colour at (255, 255, 255)
		colour.red = std::min(colour.red, 255);
		colour.green = std::min(colour.green, 255);
		colour.blue = std::min(colour.blue, 255);
		return colour;
	}
	return {0, 0, 0};
}

float calculateLightIntensity(std::map<std::string, Model> &scene, Light &light, RayTriangleIntersection &intersection) {
	float intensity = 0;
	if (light.type == POINT)
		intensity = calculateShadows(scene, light.position, intersection.intersectionPoint, intersection.intersectedModel.name);
	else if (light.type == AREA) {
		for (int u = 0; u < light.uSize; u++) {
			for (int v = 0; v < light.vSize; v++) {
				glm::vec3 lightPosition = light.position + light.uStep * (u + std::rand() / float(RAND_MAX)) + light.vStep * (v + std::rand() / float(RAND_MAX));
				intensity += calculateShadows(scene, lightPosition, intersection.intersectionPoint, intersection.intersectedModel.name);
			}
		}
		intensity /= (light.vSize * light.uSize);
	}
	return intensity;
}

Colour castRay(std::map<std::string, Model> &scene, glm::vec3 origin, glm::vec3 direction, Light &light, int depth, std::string originObjName, ModelTriangle *tri) {
	float tSmallest = MAXFLOAT;
	RayTriangleIntersection intersection;

	for (auto &pair : scene) {
		Model &model = pair.second;
		if (depth > 0 && originObjName != "" && model.name == originObjName) continue;	// don't want to hit ourselves (for mirrors etc.)
		for (int i = 0; i < model.triangles.size(); i++) {
			ModelTriangle &triangle = model.triangles.at(i);
			if (tri != nullptr && tri == &triangle) continue;  // for glass !
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
					intersection.intersectedModel = model;
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
		float lightIntensity = calculateLightIntensity(scene, light, intersection);
		switch (intersection.intersectedModel.type) {
			case FLAT:
				colour = diffuseAmbientShading(light, direction, intersection, lightIntensity);
				break;
			case FLAT_SPECULAR:
				colour = diffuseAmbientShading(light, direction, intersection, lightIntensity);
				colour = specularShading(light, direction, intersection, lightIntensity);
				break;
			case SMOOTH_GOURAUD:
				colour = gouraudShading(light, direction, intersection, lightIntensity);
				break;
			case SMOOTH_PHONG:
				colour = phongShading(light, direction, intersection, lightIntensity);
				break;
			case MIRROR:
				colour = mirror(scene, light, direction, intersection, depth, 10, lightIntensity);
				break;
			case MIRROR_PHONG:
				colour = mirrorPhong(scene, light, direction, intersection, depth, 10, lightIntensity);
				break;
			case GLASS: {
				float refractionIndex = 1.5;
				colour = transparentShading(scene, light, direction, intersection, refractionIndex, depth, 20, lightIntensity);
				break;
			}
			case GLASS_PHONG: {
				float refractionIndex = 1.5;
				colour = transparentShadingPhong(scene, light, direction, intersection, refractionIndex, depth, 10, lightIntensity);
				break;
			}
			case LIGHT:
				colour = {255, 255, 255};
		}
		// }
		return colour;
	}
	return {0, 0, 0};
}

void raytrace(DrawingWindow &window, std::map<std::string, Model> &scene, Camera &camera, Light &light) {
	// TODO: help for perspective-correct textures: jacob pro computer graphics RasterisedRenderer.cpp
	for (int h = 0; h < window.height; h++) {
		for (int w = 0; w < window.width; w++) {
			// (de)scale by 100
			float worldX = (w - window.width / 2.0f) / 100;
			float worldY = -(h - window.height / 2.0f) / 100;
			glm::vec3 rayDestination = {worldX, worldY, -camera.focalLength};
			// apply camera rotation
			glm::vec3 rayDir = glm::normalize(rayDestination) * glm::inverse(camera.orientation);

			Colour colour = castRay(scene, camera.position, rayDir, light);
			draw(window, w, h, colour);
		}
	}
}

// void animate1(int &frameCount) {
// 	RenderMethod renderMethod = WIREFRAME;
// 	bool finished = true;
// 	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
// 	SDL_Event event;
//
// 	// load models
// 	std::map<std::string, Model> scene;
// 	scene.emplace("redBox", Model{"red-box.obj", "cornell-box.mtl", 0.35, "redBox", FLAT_SPECULAR, true});
// 	scene.emplace("blueBox", Model{"blue-box.obj", "cornell-box.mtl", 0.35, "blueBox", FLAT_SPECULAR, true});
// 	scene.emplace("leftWall", Model{"left-wall.obj", "cornell-box.mtl", 0.35, "leftWall", FLAT_SPECULAR, true});
// 	scene.emplace("rightWall", Model{"right-wall.obj", "cornell-box.mtl", 0.35, "rightWall", FLAT_SPECULAR, true});
// 	scene.emplace("backWall", Model{"back-wall.obj", "cornell-box.mtl", 0.35, "backWall", FLAT_SPECULAR, false});
// 	scene.emplace("ceiling", Model{"ceiling.obj", "cornell-box.mtl", 0.35, "ceiling", FLAT_SPECULAR, true});
// 	scene.emplace("floor", Model{"floor.obj", "cornell-box.mtl", 0.35, "floor", FLAT_SPECULAR, true});
// 	scene.emplace("bunny", Model{"bunny-low.obj", "", 0.007, "bunny", FLAT_SPECULAR, true});
// 	// set up bunny
// 	scene["bunny"].rotate(90, 0, 0);
// 	scene["bunny"].translate(0.08, -0.43, 0.32);
// 	for (auto &tri : scene["bunny"].triangles)
// 		tri.colour = {150, 150, 150};
//
// 	float focalLength = 4;
// 	Camera camera{focalLength};
// 	camera.translate(0, 0, 4);
//
// 	Light light{10, 0.3, POINT, 60, 60, {0.4, 0, 0}, {0, 0, 0.4}};
// 	light.translate(0, 0.8, 0.7);
//
// 	scene["backWall"].addTransformation(WAIT, 0, 0, 0, 8, 1);
// 	scene["backWall"].addTransformation(SHADOWS, 0, 0, 0, 0, 1);
// 	for (auto &pair : scene) {
// 		auto &model = pair.second;
// 		model.addTransformation(ROTATE, 0, 0, 360, 5, 0);
// 		model.addTransformation(WAIT, 0, 0, 0, 3);
// 		model.addTransformation(ROTATE, 0, 360, 0, 5, 0);
// 	}
// 	scene["blueBox"].addTransformation(WAIT, 0, 0, 0, 3, 0);
// 	scene["blueBox"].addTransformation(MIRROR_, 0, 0, 0, 0, 0);
// 	scene["blueBox"].addTransformation(WAIT, 0, 0, 0, 6, 0);
// 	scene["blueBox"].addTransformation(SHADOWS, 0, 0, 0, 0, 0);
// 	scene["blueBox"].addTransformation(GLASS_, 0, 0, 0, 0, 0);
// 	scene["blueBox"].addTransformation(WAIT, 0, 0, 0, 6, 0);
// 	scene["blueBox"].addTransformation(FLAT_SPECULAR_, 0, 0, 0, 0, 0);
// 	scene["blueBox"].addTransformation(SHADOWS, 0, 0, 0, 0, 0);
//
// 	camera.addTransformation(WAIT, 0, 0, 0, 5, 0);
// 	// switch to rasterise
// 	camera.addTransformation(SWITCH_RENDERING_METHOD, 0, 0, 0, 0, 0);
// 	camera.addTransformation(WAIT, 0, 0, 0, 8, 0);
// 	// switch to raytrace
// 	camera.addTransformation(SWITCH_RENDERING_METHOD, 0, 0, 0, 0, 0);
// 	// move in for mirror
// 	camera.addTransformation(WAIT, 0, 0, 0, 4, 0);
// 	camera.addTransformation(WAIT, 0, 0, 0, 14, 1);
// 	camera.addTransformation(ROTATE, -27, 28, 0, 2, 1);
// 	camera.addTransformation(WAIT, 0, 0, 0, 1, 1);
// 	camera.addTransformation(ROTATE, 27, -28, 0, 2, 1);
// 	camera.addTransformation(WAIT, 0, 0, 0, 1, 1);
//
// 	camera.addTransformation(TRANSLATE, -1, -0.8, -1.4, 2, 0);
// 	camera.addTransformation(WAIT, 0, 0, 0, 1, 0);
// 	camera.addTransformation(TRANSLATE, 1, 0.8, 1.4, 2, 0);
// 	camera.addTransformation(WAIT, 0, 0, 0, 1, 0);
// 	// move in for glass
// 	camera.addTransformation(ROTATE, 27, 0, 0, 2, 1);
// 	camera.addTransformation(WAIT, 0, 0, 0, 1, 1);
// 	camera.addTransformation(ROTATE, -27, 0, 0, 2, 1);
// 	camera.addTransformation(WAIT, 0, 0, 0, 1, 1);
//
// 	camera.addTransformation(TRANSLATE, 0, 1, -1.4, 2, 0);
// 	camera.addTransformation(WAIT, 0, 0, 0, 1, 0);
// 	camera.addTransformation(TRANSLATE, 0, -1, 1.4, 2, 0);
// 	camera.addTransformation(WAIT, 0, 0, 0, 1, 0);
// 	// move in for soft shadows
// 	camera.addTransformation(ROTATE, 27, -28, 0, 2, 1);
// 	camera.addTransformation(WAIT, 0, 0, 0, 1, 1);
// 	camera.addTransformation(ROTATE, -27, 28, 0, 2, 1);
// 	camera.addTransformation(WAIT, 0, 0, 0, 1, 1);
//
// 	camera.addTransformation(TRANSLATE, 1, 0.8, -1.4, 2, 0);
// 	camera.addTransformation(WAIT, 0, 0, 0, 1, 0);
// 	camera.addTransformation(TRANSLATE, -1, -0.8, 1.4, 2, 0);
// 	camera.addTransformation(WAIT, 0, 0, 0, 1, 0);
//
// 	light.addTransformation(WAIT, 0, 0, 0, 29, 0);
// 	light.addTransformation(AREALIGHT, 0, 0, 0, 0, 0);
//
// 	while (true) {
// 		window.clearPixels();
// 		for (auto &pair : scene) {
// 			Model &model = pair.second;
// 			if (!model.transformations0.empty()) {
// 				finished = false;
// 				auto t = model.transformations0.begin();
// 				if (t->type == ROTATE) model.rotate(t->x, t->y, t->z);
// 				if (t->type == TRANSLATE) model.translate(t->x, t->y, t->z);
// 				if (t->type == SCALE) model.scale(t->x, t->y, t->z);
// 				if (t->type >= FLAT_ && t->type <= LIGHT_)
// 					model.type = (ModelType)t->type;
// 				if (t->type == SHADOWS) model.shadows = !model.shadows;
// 				model.transformations0.erase(t);
// 			}
// 			if (!model.transformations1.empty()) {
// 				finished = false;
// 				auto t = model.transformations1.begin();
// 				if (t->type == ROTATE) model.rotate(t->x, t->y, t->z);
// 				if (t->type == TRANSLATE) model.translate(t->x, t->y, t->z);
// 				if (t->type == SCALE) model.scale(t->x, t->y, t->z);
// 				if (t->type >= FLAT_ && t->type <= LIGHT_)
// 					model.type = (ModelType)t->type;
// 				if (t->type == SHADOWS) model.shadows = !model.shadows;
// 				model.transformations1.erase(t);
// 			}
// 			if (!model.transformations2.empty()) {
// 				finished = false;
// 				auto t = model.transformations2.begin();
// 				if (t->type == ROTATE) model.rotate(t->x, t->y, t->z);
// 				if (t->type == TRANSLATE) model.translate(t->x, t->y, t->z);
// 				if (t->type == SCALE) model.scale(t->x, t->y, t->z);
// 				if (t->type >= FLAT_ && t->type <= LIGHT_)
// 					model.type = (ModelType)t->type;
// 				if (t->type == SHADOWS) model.shadows = !model.shadows;
// 				model.transformations2.erase(t);
// 			}
// 		}
// 		if (!light.transformations0.empty()) {
// 			finished = false;
// 			auto t = light.transformations0.begin();
// 			if (t->type == ROTATE) light.rotate(t->x, t->y, t->z);
// 			if (t->type == TRANSLATE) light.translate(t->x, t->y, t->z);
// 			if (t->type == POINTLIGHT) {
// 				light.type = POINT;
// 				light.position = light.position + (light.uVec / 2.0f) + (light.vVec / 2.0f);
// 			}
// 			if (t->type == AREALIGHT) {
// 				light.type = AREA;
// 				light.position = light.position - (light.uVec / 2.0f) - (light.vVec / 2.0f);
// 			}
// 			light.transformations0.erase(t);
// 		}
// 		if (!light.transformations1.empty()) {
// 			finished = false;
// 			auto t = light.transformations1.begin();
// 			if (t->type == ROTATE) light.rotate(t->x, t->y, t->z);
// 			if (t->type == TRANSLATE) light.translate(t->x, t->y, t->z);
// 			if (t->type == POINTLIGHT) {
// 				light.type = POINT;
// 				light.position = light.position + (light.uVec / 2.0f) + (light.vVec / 2.0f);
// 			}
// 			if (t->type == AREALIGHT) {
// 				light.type = AREA;
// 				light.position = light.position - (light.uVec / 2.0f) - (light.vVec / 2.0f);
// 			}
// 			light.transformations1.erase(t);
// 		}
// 		if (!light.transformations2.empty()) {
// 			finished = false;
// 			auto t = light.transformations2.begin();
// 			if (t->type == ROTATE) light.rotate(t->x, t->y, t->z);
// 			if (t->type == TRANSLATE) light.translate(t->x, t->y, t->z);
// 			if (t->type == POINTLIGHT) {
// 				light.type = POINT;
// 				light.position = light.position + (light.uVec / 2.0f) + (light.vVec / 2.0f);
// 			}
// 			if (t->type == AREALIGHT) {
// 				light.type = AREA;
// 				light.position = light.position - (light.uVec / 2.0f) - (light.vVec / 2.0f);
// 			}
// 			light.transformations2.erase(t);
// 		}
// 		if (!camera.transformations0.empty()) {
// 			finished = false;
// 			auto t = camera.transformations0.begin();
// 			if (t->type == ROTATE) camera.rotate(t->x, t->y, t->z);
// 			if (t->type == TRANSLATE) camera.translate(t->x, t->y, t->z);
// 			if (t->type == ROTATEPOSITION) {
// 				camera.rotatePosition(t->x, t->y, t->z);
// 				camera.lookat(0, 0, 0);
// 			}
// 			if (t->type == SWITCH_RENDERING_METHOD) {
// 				if (renderMethod == WIREFRAME)
// 					renderMethod = RASTERISE;
// 				else if (renderMethod == RASTERISE)
// 					renderMethod = RAYTRACE;
// 				else if (renderMethod == RAYTRACE)
// 					renderMethod = WIREFRAME;
// 			}
// 			camera.transformations0.erase(t);
// 		}
// 		if (!camera.transformations1.empty()) {
// 			finished = false;
// 			auto t = camera.transformations1.begin();
// 			if (t->type == ROTATE) camera.rotate(t->x, t->y, t->z);
// 			if (t->type == TRANSLATE) camera.translate(t->x, t->y, t->z);
// 			if (t->type == ROTATEPOSITION) {
// 				camera.rotatePosition(t->x, t->y, t->z);
// 				camera.lookat(0, 0, 0);
// 			}
// 			if (t->type == SWITCH_RENDERING_METHOD) {
// 				if (renderMethod == WIREFRAME)
// 					renderMethod = RASTERISE;
// 				else if (renderMethod == RASTERISE)
// 					renderMethod = RAYTRACE;
// 				else if (renderMethod == RAYTRACE)
// 					renderMethod = WIREFRAME;
// 			}
// 			camera.transformations1.erase(t);
// 		}
// 		if (!camera.transformations2.empty()) {
// 			finished = false;
// 			auto t = camera.transformations2.begin();
// 			if (t->type == ROTATE) camera.rotate(t->x, t->y, t->z);
// 			if (t->type == TRANSLATE) camera.translate(t->x, t->y, t->z);
// 			if (t->type == ROTATEPOSITION) {
// 				camera.rotatePosition(t->x, t->y, t->z);
// 				camera.lookat(0, 0, 0);
// 			}
// 			if (t->type == SWITCH_RENDERING_METHOD) {
// 				if (renderMethod == WIREFRAME)
// 					renderMethod = RASTERISE;
// 				else if (renderMethod == RASTERISE)
// 					renderMethod = RAYTRACE;
// 				else if (renderMethod == RAYTRACE)
// 					renderMethod = WIREFRAME;
// 			}
// 			camera.transformations2.erase(t);
// 		}
// 		if (finished) break;
// 		if (renderMethod == RASTERISE)
// 			rasterise(window, scene, camera);
// 		else if (renderMethod == RAYTRACE)
// 			raytrace(window, scene, camera, light);
// 		else if (renderMethod == WIREFRAME)
// 			wireframe(window, scene, camera);
// 		// We MUST poll for events - otherwise the window will freeze !
// 		if (window.pollForInputEvents(event)) handleEvent(event, window, camera);
// 		// Need to render the frame at the end, or nothing actually gets shown on the screen !
// 		window.renderFrame();
// 		std::stringstream ss;
// 		ss << std::setfill('0') << std::setw(6) << frameCount;
// 		window.saveBMP("output" + ss.str() + ".bmp");
// 		finished = true;
// 		frameCount++;
// 	}
// }
//
void animate2(int &frameCount) {
	RenderMethod renderMethod = RAYTRACE;
	bool finished = true;
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;

	// load models
	std::map<std::string, Model> scene;
	scene.emplace("leftWall", Model{"left-wall.obj", "cornell-box.mtl", 0.35, "leftWall", FLAT_SPECULAR, true});
	scene.emplace("rightWall", Model{"right-wall.obj", "cornell-box.mtl", 0.35, "rightWall", FLAT_SPECULAR, true});
	scene.emplace("backWall", Model{"back-wall.obj", "cornell-box.mtl", 0.35, "backWall", MIRROR, true});
	scene.emplace("ceiling", Model{"ceiling.obj", "cornell-box.mtl", 0.35, "ceiling", FLAT_SPECULAR, true});
	scene.emplace("floor", Model{"floor.obj", "cornell-box.mtl", 0.35, "floor", FLAT_SPECULAR, true});
	scene.emplace("bunny", Model{"bunny-low.obj", "", 0.012, "bunny", MIRROR, true});
	// set up bunny
	scene["bunny"].rotate(90, 0, 0);
	scene["bunny"].translate(-0.4, -1, 0);

	float focalLength = 4;
	Camera camera{focalLength};
	camera.translate(0, 0, 4);

	Light light{10, 0.3, POINT, 60, 60, {0.4, 0, 0}, {0, 0, 0.4}};
	light.translate(0, 0.8, 0.7);

	// light.addTransformation(TRANSLATE, 0.8, 0, 0, 2, 0);
	// light.addTransformation(TRANSLATE, -1.6, 0, 0, 4, 0);
	// light.addTransformation(TRANSLATE, 0.8, 0, 0, 2, 0);

	camera.addTransformation(ROTATE, 27, -28, 0, 1.5, 1);
	camera.addTransformation(WAIT, 0, 0, 0, 0.5, 1);
	camera.addTransformation(ROTATE, -27, 28, 0, 1.5, 1);
	camera.addTransformation(WAIT, 0, 0, 0, 0.5, 1);

	camera.addTransformation(TRANSLATE, 1, 0.8, -1.4, 1.5, 0);
	camera.addTransformation(WAIT, 0, 0, 0, 0.5, 0);
	camera.addTransformation(TRANSLATE, -1, -0.8, 1.4, 1.5, 0);
	camera.addTransformation(WAIT, 0, 0, 0, 0.5, 0);

	camera.addTransformation(ROTATE, 27, 28, 0, 1.5, 1);
	camera.addTransformation(WAIT, 0, 0, 0, 0.5, 1);
	camera.addTransformation(ROTATE, -27, -28, 0, 1.5, 1);
	camera.addTransformation(WAIT, 0, 0, 0, 0.5, 1);

	camera.addTransformation(TRANSLATE, -1, 0.8, -1.4, 1.5, 0);
	camera.addTransformation(WAIT, 0, 0, 0, 0.5, 0);
	camera.addTransformation(TRANSLATE, 1, -0.8, 1.4, 1.5, 0);
	camera.addTransformation(WAIT, 0, 0, 0, 0.5, 0);

	scene["bunny"].addTransformation(WAIT, 0, 0, 0, 4, 0);
	scene["bunny"].addTransformation(SHADOWS, 0, 0, 0, 0, 0);
	scene["bunny"].addTransformation(GLASS_, 0, 0, 0, 0, 0);

	while (true) {
		window.clearPixels();
		for (auto &pair : scene) {
			Model &model = pair.second;
			if (!model.transformations0.empty()) {
				finished = false;
				auto t = model.transformations0.begin();
				if (t->type == ROTATE) model.rotate(t->x, t->y, t->z);
				if (t->type == TRANSLATE) model.translate(t->x, t->y, t->z);
				if (t->type == SCALE) model.scale(t->x, t->y, t->z);
				if (t->type >= FLAT_ && t->type <= LIGHT_)
					model.type = (ModelType)t->type;
				if (t->type == SHADOWS) model.shadows = !model.shadows;
				model.transformations0.erase(t);
			}
			if (!model.transformations1.empty()) {
				finished = false;
				auto t = model.transformations1.begin();
				if (t->type == ROTATE) model.rotate(t->x, t->y, t->z);
				if (t->type == TRANSLATE) model.translate(t->x, t->y, t->z);
				if (t->type == SCALE) model.scale(t->x, t->y, t->z);
				if (t->type >= FLAT_ && t->type <= LIGHT_)
					model.type = (ModelType)t->type;
				if (t->type == SHADOWS) model.shadows = !model.shadows;
				model.transformations1.erase(t);
			}
			if (!model.transformations2.empty()) {
				finished = false;
				auto t = model.transformations2.begin();
				if (t->type == ROTATE) model.rotate(t->x, t->y, t->z);
				if (t->type == TRANSLATE) model.translate(t->x, t->y, t->z);
				if (t->type == SCALE) model.scale(t->x, t->y, t->z);
				if (t->type >= FLAT_ && t->type <= LIGHT_)
					model.type = (ModelType)t->type;
				if (t->type == SHADOWS) model.shadows = !model.shadows;
				model.transformations2.erase(t);
			}
		}
		if (!light.transformations0.empty()) {
			finished = false;
			auto t = light.transformations0.begin();
			if (t->type == ROTATE) light.rotate(t->x, t->y, t->z);
			if (t->type == TRANSLATE) light.translate(t->x, t->y, t->z);
			if (t->type == POINTLIGHT) {
				light.type = POINT;
				light.position = light.position + (light.uVec / 2.0f) + (light.vVec / 2.0f);
			}
			if (t->type == AREALIGHT) {
				light.type = AREA;
				light.position = light.position - (light.uVec / 2.0f) - (light.vVec / 2.0f);
			}
			light.transformations0.erase(t);
		}
		if (!light.transformations1.empty()) {
			finished = false;
			auto t = light.transformations1.begin();
			if (t->type == ROTATE) light.rotate(t->x, t->y, t->z);
			if (t->type == TRANSLATE) light.translate(t->x, t->y, t->z);
			if (t->type == POINTLIGHT) {
				light.type = POINT;
				light.position = light.position + (light.uVec / 2.0f) + (light.vVec / 2.0f);
			}
			if (t->type == AREALIGHT) {
				light.type = AREA;
				light.position = light.position - (light.uVec / 2.0f) - (light.vVec / 2.0f);
			}
			light.transformations1.erase(t);
		}
		if (!light.transformations2.empty()) {
			finished = false;
			auto t = light.transformations2.begin();
			if (t->type == ROTATE) light.rotate(t->x, t->y, t->z);
			if (t->type == TRANSLATE) light.translate(t->x, t->y, t->z);
			if (t->type == POINTLIGHT) {
				light.type = POINT;
				light.position = light.position + (light.uVec / 2.0f) + (light.vVec / 2.0f);
			}
			if (t->type == AREALIGHT) {
				light.type = AREA;
				light.position = light.position - (light.uVec / 2.0f) - (light.vVec / 2.0f);
			}
			light.transformations2.erase(t);
		}
		if (!camera.transformations0.empty()) {
			finished = false;
			auto t = camera.transformations0.begin();
			if (t->type == ROTATE) camera.rotate(t->x, t->y, t->z);
			if (t->type == TRANSLATE) camera.translate(t->x, t->y, t->z);
			if (t->type == ROTATEPOSITION) {
				camera.rotatePosition(t->x, t->y, t->z);
				camera.lookat(0, 0, 0);
			}
			if (t->type == SWITCH_RENDERING_METHOD) {
				if (renderMethod == WIREFRAME)
					renderMethod = RASTERISE;
				else if (renderMethod == RASTERISE)
					renderMethod = RAYTRACE;
				else if (renderMethod == RAYTRACE)
					renderMethod = WIREFRAME;
			}
			camera.transformations0.erase(t);
		}
		if (!camera.transformations1.empty()) {
			finished = false;
			auto t = camera.transformations1.begin();
			if (t->type == ROTATE) camera.rotate(t->x, t->y, t->z);
			if (t->type == TRANSLATE) camera.translate(t->x, t->y, t->z);
			if (t->type == ROTATEPOSITION) {
				camera.rotatePosition(t->x, t->y, t->z);
				camera.lookat(0, 0, 0);
			}
			if (t->type == SWITCH_RENDERING_METHOD) {
				if (renderMethod == WIREFRAME)
					renderMethod = RASTERISE;
				else if (renderMethod == RASTERISE)
					renderMethod = RAYTRACE;
				else if (renderMethod == RAYTRACE)
					renderMethod = WIREFRAME;
			}
			camera.transformations1.erase(t);
		}
		if (!camera.transformations2.empty()) {
			finished = false;
			auto t = camera.transformations2.begin();
			if (t->type == ROTATE) camera.rotate(t->x, t->y, t->z);
			if (t->type == TRANSLATE) camera.translate(t->x, t->y, t->z);
			if (t->type == ROTATEPOSITION) {
				camera.rotatePosition(t->x, t->y, t->z);
				camera.lookat(0, 0, 0);
			}
			if (t->type == SWITCH_RENDERING_METHOD) {
				if (renderMethod == WIREFRAME)
					renderMethod = RASTERISE;
				else if (renderMethod == RASTERISE)
					renderMethod = RAYTRACE;
				else if (renderMethod == RAYTRACE)
					renderMethod = WIREFRAME;
			}
			camera.transformations2.erase(t);
		}
		if (finished) break;
		if (renderMethod == RASTERISE)
			rasterise(window, scene, camera);
		else if (renderMethod == RAYTRACE)
			raytrace(window, scene, camera, light);
		else if (renderMethod == WIREFRAME)
			wireframe(window, scene, camera);
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window, camera);
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
		std::stringstream ss;
		ss << std::setfill('0') << std::setw(6) << frameCount;
		window.saveBMP("output" + ss.str() + ".bmp");
		finished = true;
		frameCount++;
	}
}

// void animate3(int &frameCount) {
// 	RenderMethod renderMethod = RAYTRACE;
// 	bool finished = true;
// 	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
// 	SDL_Event event;
//
// 	// load models
// 	std::map<std::string, Model> scene;
// 	scene.emplace("leftWall", Model{"left-wall.obj", "cornell-box.mtl", 0.35, "leftWall", FLAT_SPECULAR, true});
// 	scene.emplace("rightWall", Model{"right-wall.obj", "cornell-box.mtl", 0.35, "rightWall", FLAT_SPECULAR, true});
// 	scene.emplace("backWall", Model{"back-wall.obj", "cornell-box.mtl", 0.35, "backWall", FLAT_SPECULAR, true});
// 	scene.emplace("ceiling", Model{"ceiling.obj", "cornell-box.mtl", 0.35, "ceiling", FLAT_SPECULAR, true});
// 	scene.emplace("floor", Model{"floor.obj", "cornell-box.mtl", 0.35, "floor", FLAT_SPECULAR, true});
// 	scene.emplace("redBox", Model{"red-box.obj", "cornell-box.mtl", 0.35, "redBox", FLAT_SPECULAR, true});
// 	scene.emplace("blueBox", Model{"blue-box.obj", "cornell-box.mtl", 0.35, "blueBox", FLAT_SPECULAR, true});
// 	// scene.emplace("sphere", Model{"sphere.obj", "", 0.4, "sphere", SMOOTH_GOURAUD, true});
// 	// scene["sphere"].translate(0, -0.9, -0.15);
// 	// scene["sphere"].rotate(0, 1, 0);
//
// 	float focalLength = 4;
// 	Camera camera{focalLength};
// 	camera.translate(0, 0, 4);
//
// 	Light light{10, 0.3, AREA, 40, 40, {0.4, 0, 0}, {0, 0, 0.4}};
// 	light.translate(0, 0.7, 0.5);
// 	light.addTransformation(ROTATE, 0, 360, 0, 4, 0);
//
// 	// scene["sphere"].addTransformation(WAIT, 0, 0, 0, 4, 0);
// 	// scene["sphere"].addTransformation(SMOOTH_PHONG_, 0, 0, 0, 0, 0);
// 	// scene["sphere"].addTransformation(WAIT, 0, 0, 0, 4, 0);
// 	// scene["sphere"].addTransformation(MIRROR_PHONG_, 0, 0, 0, 0, 0);
// 	// scene["sphere"].addTransformation(WAIT, 0, 0, 0, 6, 0);
// 	// scene["sphere"].addTransformation(SHADOWS, 0, 0, 0, 0, 0);
// 	// scene["sphere"].addTransformation(GLASS_PHONG_, 0, 0, 0, 0, 0);
// 	//
// 	// camera.addTransformation(WAIT, 0, 0, 0, 8, 0);
// 	//
// 	// camera.addTransformation(TRANSLATE, 0, 0, -1.8, 2, 0);
// 	// camera.addTransformation(WAIT, 0, 0, 0, 1, 0);
// 	// camera.addTransformation(TRANSLATE, 0, 0, 1.8, 2, 0);
// 	// camera.addTransformation(WAIT, 0, 0, 0, 1, 0);
// 	//
// 	// camera.addTransformation(TRANSLATE, 0, 0, -1.8, 2, 0);
// 	// camera.addTransformation(WAIT, 0, 0, 0, 1, 0);
// 	// camera.addTransformation(TRANSLATE, 0, 0, 1.8, 2, 0);
// 	// camera.addTransformation(WAIT, 0, 0, 0, 1, 0);
//
// 	while (true) {
// 		window.clearPixels();
// 		for (auto &pair : scene) {
// 			Model &model = pair.second;
// 			if (!model.transformations0.empty()) {
// 				finished = false;
// 				auto t = model.transformations0.begin();
// 				if (t->type == ROTATE) model.rotate(t->x, t->y, t->z);
// 				if (t->type == TRANSLATE) model.translate(t->x, t->y, t->z);
// 				if (t->type == SCALE) model.scale(t->x, t->y, t->z);
// 				if (t->type >= FLAT_ && t->type <= LIGHT_)
// 					model.type = (ModelType)t->type;
// 				if (t->type == SHADOWS) model.shadows = !model.shadows;
// 				model.transformations0.erase(t);
// 			}
// 			if (!model.transformations1.empty()) {
// 				finished = false;
// 				auto t = model.transformations1.begin();
// 				if (t->type == ROTATE) model.rotate(t->x, t->y, t->z);
// 				if (t->type == TRANSLATE) model.translate(t->x, t->y, t->z);
// 				if (t->type == SCALE) model.scale(t->x, t->y, t->z);
// 				if (t->type >= FLAT_ && t->type <= LIGHT_)
// 					model.type = (ModelType)t->type;
// 				if (t->type == SHADOWS) model.shadows = !model.shadows;
// 				model.transformations1.erase(t);
// 			}
// 			if (!model.transformations2.empty()) {
// 				finished = false;
// 				auto t = model.transformations2.begin();
// 				if (t->type == ROTATE) model.rotate(t->x, t->y, t->z);
// 				if (t->type == TRANSLATE) model.translate(t->x, t->y, t->z);
// 				if (t->type == SCALE) model.scale(t->x, t->y, t->z);
// 				if (t->type >= FLAT_ && t->type <= LIGHT_)
// 					model.type = (ModelType)t->type;
// 				if (t->type == SHADOWS) model.shadows = !model.shadows;
// 				model.transformations2.erase(t);
// 			}
// 		}
// 		if (!light.transformations0.empty()) {
// 			finished = false;
// 			auto t = light.transformations0.begin();
// 			if (t->type == ROTATE) light.rotate(t->x, t->y, t->z);
// 			if (t->type == TRANSLATE) light.translate(t->x, t->y, t->z);
// 			if (t->type == POINTLIGHT) {
// 				light.type = POINT;
// 				light.position = light.position + (light.uVec / 2.0f) + (light.vVec / 2.0f);
// 			}
// 			if (t->type == AREALIGHT) {
// 				light.type = AREA;
// 				light.position = light.position - (light.uVec / 2.0f) - (light.vVec / 2.0f);
// 			}
// 			light.transformations0.erase(t);
// 		}
// 		if (!light.transformations1.empty()) {
// 			finished = false;
// 			auto t = light.transformations1.begin();
// 			if (t->type == ROTATE) light.rotate(t->x, t->y, t->z);
// 			if (t->type == TRANSLATE) light.translate(t->x, t->y, t->z);
// 			if (t->type == POINTLIGHT) {
// 				light.type = POINT;
// 				light.position = light.position + (light.uVec / 2.0f) + (light.vVec / 2.0f);
// 			}
// 			if (t->type == AREALIGHT) {
// 				light.type = AREA;
// 				light.position = light.position - (light.uVec / 2.0f) - (light.vVec / 2.0f);
// 			}
// 			light.transformations1.erase(t);
// 		}
// 		if (!light.transformations2.empty()) {
// 			finished = false;
// 			auto t = light.transformations2.begin();
// 			if (t->type == ROTATE) light.rotate(t->x, t->y, t->z);
// 			if (t->type == TRANSLATE) light.translate(t->x, t->y, t->z);
// 			if (t->type == POINTLIGHT) {
// 				light.type = POINT;
// 				light.position = light.position + (light.uVec / 2.0f) + (light.vVec / 2.0f);
// 			}
// 			if (t->type == AREALIGHT) {
// 				light.type = AREA;
// 				light.position = light.position - (light.uVec / 2.0f) - (light.vVec / 2.0f);
// 			}
// 			light.transformations2.erase(t);
// 		}
// 		if (!camera.transformations0.empty()) {
// 			finished = false;
// 			auto t = camera.transformations0.begin();
// 			if (t->type == ROTATE) camera.rotate(t->x, t->y, t->z);
// 			if (t->type == TRANSLATE) camera.translate(t->x, t->y, t->z);
// 			if (t->type == ROTATEPOSITION) {
// 				camera.rotatePosition(t->x, t->y, t->z);
// 				camera.lookat(0, 0, 0);
// 			}
// 			if (t->type == SWITCH_RENDERING_METHOD) {
// 				if (renderMethod == WIREFRAME)
// 					renderMethod = RASTERISE;
// 				else if (renderMethod == RASTERISE)
// 					renderMethod = RAYTRACE;
// 				else if (renderMethod == RAYTRACE)
// 					renderMethod = WIREFRAME;
// 			}
// 			camera.transformations0.erase(t);
// 		}
// 		if (!camera.transformations1.empty()) {
// 			finished = false;
// 			auto t = camera.transformations1.begin();
// 			if (t->type == ROTATE) camera.rotate(t->x, t->y, t->z);
// 			if (t->type == TRANSLATE) camera.translate(t->x, t->y, t->z);
// 			if (t->type == ROTATEPOSITION) {
// 				camera.rotatePosition(t->x, t->y, t->z);
// 				camera.lookat(0, 0, 0);
// 			}
// 			if (t->type == SWITCH_RENDERING_METHOD) {
// 				if (renderMethod == WIREFRAME)
// 					renderMethod = RASTERISE;
// 				else if (renderMethod == RASTERISE)
// 					renderMethod = RAYTRACE;
// 				else if (renderMethod == RAYTRACE)
// 					renderMethod = WIREFRAME;
// 			}
// 			camera.transformations1.erase(t);
// 		}
// 		if (!camera.transformations2.empty()) {
// 			finished = false;
// 			auto t = camera.transformations2.begin();
// 			if (t->type == ROTATE) camera.rotate(t->x, t->y, t->z);
// 			if (t->type == TRANSLATE) camera.translate(t->x, t->y, t->z);
// 			if (t->type == ROTATEPOSITION) {
// 				camera.rotatePosition(t->x, t->y, t->z);
// 				camera.lookat(0, 0, 0);
// 			}
// 			if (t->type == SWITCH_RENDERING_METHOD) {
// 				if (renderMethod == WIREFRAME)
// 					renderMethod = RASTERISE;
// 				else if (renderMethod == RASTERISE)
// 					renderMethod = RAYTRACE;
// 				else if (renderMethod == RAYTRACE)
// 					renderMethod = WIREFRAME;
// 			}
// 			camera.transformations2.erase(t);
// 		}
// 		if (finished) break;
// 		if (renderMethod == RASTERISE)
// 			rasterise(window, scene, camera);
// 		else if (renderMethod == RAYTRACE)
// 			raytrace(window, scene, camera, light);
// 		else if (renderMethod == WIREFRAME)
// 			wireframe(window, scene, camera);
// 		// We MUST poll for events - otherwise the window will freeze !
// 		if (window.pollForInputEvents(event)) handleEvent(event, window, camera);
// 		// Need to render the frame at the end, or nothing actually gets shown on the screen !
// 		window.renderFrame();
// 		std::stringstream ss;
// 		ss << std::setfill('0') << std::setw(6) << frameCount;
// 		window.saveBMP("output" + ss.str() + ".bmp");
// 		finished = true;
// 		frameCount++;
// 	}
// }

int main(int argc, char *argv[]) {
	int frameCount = 0;
	animate2(frameCount);
}
