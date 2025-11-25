#include <CanvasPoint.h>
#include <CanvasTriangle.h>
#include <Colour.h>
#include <DrawingWindow.h>
#include <ModelTriangle.h>
#include <Utils.h>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include "Camera.hpp"
#include "Draw.hpp"
#include "Light.hpp"
#include "Model.hpp"
#include "RayTriangleIntersection.h"

Colour castRay(std::vector<Model> &scene, glm::vec3 origin, glm::vec3 direction, Light &light, int depth = 0, std::string originObjName = "");

void handleEvent(SDL_Event event, DrawingWindow &window, Camera &camera, bool &rasterising, bool &raytracedOnce) {
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

CanvasPoint getCanvasIntersectionPoint(Camera &camera, glm::vec3 vertexPosition) {
	// updadte vertex position based on camera transformations
	vertexPosition = (vertexPosition - camera.position) * camera.orientation;

	// FIXME: ask TA's about scaling model and funny behaviour when scale = 100, also about black lines appearing when rotating etc
	// also about boxes going through floor - is it ok??
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

void rasterise(DrawingWindow &window, std::vector<Model> &scene, Camera &camera) {
	// reset depth buffer
	std::vector<float> depthBuffer((window.width * window.height), 0);
	for (Model &model : scene) {
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

bool calculateShadows(std::vector<Model> &scene, Light &light, glm::vec3 intersectionPoint, std::string originObjName) {
	glm::vec3 shadowRayDir = light.position - intersectionPoint;
	float distToLight = glm::length(shadowRayDir);
	// we normalise after calculating distToLight so that distToLight is actual distance,
	// but also so that t == distToLight (as the vector which t scales - shadowRayDir - has length 1 after normalisation)
	shadowRayDir = glm::normalize(shadowRayDir);

	bool needShadow = false;
	float minT = distToLight;
	ModelTriangle hitTri;

	for (Model &model : scene) {
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
					needShadow = model.shadows;
				}
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

Colour diffuseAmbientShading(Light &light, glm::vec3 rayDir, RayTriangleIntersection &intersection) {
	Colour colour = intersection.intersectedTriangle.colour;
	glm::vec3 normal = intersection.intersectedTriangle.normal;
	glm::vec3 intersectionPoint = intersection.intersectionPoint;

	float multiplier = diffuseAmbientMultiplier(light.position, light.strength, light.ambientStrength, intersectionPoint, normal);
	colour.red *= multiplier;
	colour.green *= multiplier;
	colour.blue *= multiplier;

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

Colour specularShading(Light &light, glm::vec3 rayDir, RayTriangleIntersection &intersection) {
	Colour colour = intersection.intersectedTriangle.colour;
	glm::vec3 normal = intersection.intersectedTriangle.normal;
	glm::vec3 intersectionPoint = intersection.intersectionPoint;

	// Add specular, assumes the light source is white (255, 255, 255)
	float specular = specularMultiplier(light.position, intersectionPoint, normal, rayDir, 256);
	colour.red += (255.0f * specular);
	colour.green += (255.0f * specular);
	colour.blue += (255.0f * specular);

	// Cap colour at (255, 255, 255)
	colour.red = std::min(colour.red, 255);
	colour.green = std::min(colour.green, 255);
	colour.blue = std::min(colour.blue, 255);
	return colour;
}

Colour gouraudShading(Light &light, glm::vec3 rayDir, RayTriangleIntersection &intersection) {
	ModelTriangle triangle = intersection.intersectedTriangle;
	Colour colour = triangle.colour;
	float u = intersection.u;
	float v = intersection.v;
	float lightv0 = diffuseAmbientMultiplier(light.position, light.strength, light.ambientStrength, triangle.vertices[0], triangle.vertexNormals[0]);
	float lightv1 = diffuseAmbientMultiplier(light.position, light.strength, light.ambientStrength, triangle.vertices[1], triangle.vertexNormals[1]);
	float lightv2 = diffuseAmbientMultiplier(light.position, light.strength, light.ambientStrength, triangle.vertices[2], triangle.vertexNormals[2]);
	float lightPoint = (1 - u - v) * lightv0 + u * lightv1 + v * lightv2;

	float specv0 = specularMultiplier(light.position, triangle.vertices[0], triangle.vertexNormals[0], rayDir, 16);
	float specv1 = specularMultiplier(light.position, triangle.vertices[1], triangle.vertexNormals[1], rayDir, 16);
	float specv2 = specularMultiplier(light.position, triangle.vertices[2], triangle.vertexNormals[2], rayDir, 16);
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

Colour phongShading(Light &light, glm::vec3 rayDir, RayTriangleIntersection &intersection) {
	ModelTriangle triangle = intersection.intersectedTriangle;
	Colour colour = triangle.colour;
	float u = intersection.u;
	float v = intersection.v;
	glm::vec3 intersectionNormal = (1 - u - v) * triangle.vertexNormals[0] + u * triangle.vertexNormals[1] + v * triangle.vertexNormals[2];
	intersectionNormal = glm::normalize(intersectionNormal);
	float lightPoint = diffuseAmbientMultiplier(light.position, light.strength, light.ambientStrength, intersection.intersectionPoint, intersectionNormal);
	float specPoint = specularMultiplier(light.position, intersection.intersectionPoint, intersectionNormal, rayDir, 16);
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

Colour mirror(std::vector<Model> &scene, Light &light, glm::vec3 rayDir, RayTriangleIntersection &intersection, int depth, int maxDepth, bool inShadow) {
	Colour colour;
	if (depth < maxDepth) {
		depth += 1;
		glm::vec3 reflection = glm::normalize(rayDir - 2.0f * intersection.intersectedTriangle.normal * glm::dot(rayDir, intersection.intersectedTriangle.normal));
		colour = castRay(scene, intersection.intersectionPoint, reflection, light, depth, intersection.intersectedModel.name);
	}
	// TODO: are we sure this is correct???
	if (!inShadow) {
		// calculate specular highlight
		float specular = specularMultiplier(light.position, intersection.intersectionPoint, intersection.intersectedTriangle.normal, rayDir, 512);
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

Colour mirrorPhong(std::vector<Model> &scene, Light &light, glm::vec3 rayDir, RayTriangleIntersection &intersection, int depth, int maxDepth, bool inShadow) {
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
	if (!inShadow) {
		// calculate specular highlight
		float specPoint = specularMultiplier(light.position, intersection.intersectionPoint, intersectionNormal, rayDir, 256);
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

glm::vec3 refractRay(glm::vec3 rayDir, glm::vec3 intersectionNormal, float refractionIndex) {
	float cosi = std::min(-1.0f, std::max(1.0f, glm::dot(intersectionNormal, rayDir)));	 // clamp the dot product at (-1, 1)
	float rIndexi, rIndext;																 // rIndexi is the index of refraction of the medium the ray is in before entering the second medium
	if (cosi < 0) {
		// entering refractive object
		cosi = -cosi;  // cos(theta_i) must be +ve
		rIndexi = 1;
		rIndext = refractionIndex;
	} else {
		// exiting refractive object
		intersectionNormal = -intersectionNormal;  // need to reverse normal direction
		rIndexi = refractionIndex;
		rIndext = 1;
	}
	float relativeRIndex = rIndexi / rIndext;
	float cost = 1 - std::pow(relativeRIndex, 2) * (1 - cosi * cosi);
	if (cost < 0)
		// total internal reflection. There is no refraction in this case
		return glm::vec3{0};
	else
		return relativeRIndex * rayDir + (relativeRIndex * cosi - std::sqrtf(cost)) * intersectionNormal;
}

float fresnel(glm::vec3 rayDir, glm::vec3 intersectionNormal, float refractionIndex) {
	float cosi = std::min(-1.0f, std::max(1.0f, glm::dot(intersectionNormal, rayDir)));	 // clamp the dot product at (-1, 1)
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
	if (cost < 0) {
		// total internal reflection
		return 1;
	} else {
		float rParallel = ((rIndext * cosi) - (rIndexi * cost)) / ((rIndext * cosi) + (rIndexi * cost));
		float rPerpendicular = ((rIndexi * cosi) - (rIndext * cost)) / ((rIndexi * cosi) + (rIndext * cost));
		return (std::pow(rParallel, 2) + std::pow(rPerpendicular, 2)) / 2;
	}
}

// TODO: clean up transparent, fix phong, add specular highlights to phong, do caustics/shadows??? change shadow pciker section to make it nicer
Colour transparentShading(std::vector<Model> &scene, Light &light, glm::vec3 rayDir, RayTriangleIntersection &intersection, float refractionIndex, int depth, int maxDepth, bool inShadow) {
	const float BIAS = 0.0001;
	Colour refractionColor{0, 0, 0};
	glm::vec3 normal = intersection.intersectedTriangle.normal;
	// compute fresnel
	float kr = fresnel(rayDir, normal, refractionIndex);
	bool outside = glm::dot(normal, rayDir) < 0;
	glm::vec3 biasVec = BIAS * normal;
	// compute refraction if it is not a case of total internal reflection
	if (depth < maxDepth) {
		depth += 1;
		if (kr < 1) {
			glm::vec3 refractionDir = glm::normalize(refractRay(rayDir, normal, refractionIndex));
			glm::vec3 refractionRayOrig = outside ? intersection.intersectionPoint - biasVec : intersection.intersectionPoint + biasVec;
			refractionColor = castRay(scene, refractionRayOrig, refractionDir, light, depth, "");
		}

		glm::vec3 reflectionDir = glm::normalize(rayDir - 2.0f * normal * glm::dot(rayDir, normal));
		glm::vec3 reflectionRayOrig = outside ? intersection.intersectionPoint + biasVec : intersection.intersectionPoint - biasVec;
		Colour reflectionColor = castRay(scene, reflectionRayOrig, reflectionDir, light, depth, "");

		// mix the two
		Colour colour;
		colour.red = reflectionColor.red * kr + refractionColor.red * (1 - kr);
		colour.green = reflectionColor.green * kr + refractionColor.green * (1 - kr);
		colour.blue = reflectionColor.blue * kr + refractionColor.blue * (1 - kr);
		if (!inShadow) {
			// calculate specular highlight
			float specPoint = specularMultiplier(light.position, intersection.intersectionPoint, normal, rayDir, 256);
			colour.red += (255.0f * specPoint);
			colour.green += (255.0f * specPoint);
			colour.blue += (255.0f * specPoint);
		}
		// mirrors don't always reflect 100% of light - here we pretend they only reflect 80%
		// TODO: what should I do with this??? looks kinda cool - like it's slightly grey glass
		colour.red *= 0.8;
		colour.green *= 0.8;
		colour.blue *= 0.8;

		// Cap colour at (255, 255, 255)
		colour.red = std::min(colour.red, 255);
		colour.green = std::min(colour.green, 255);
		colour.blue = std::min(colour.blue, 255);
		return colour;
	}
	// TODO: this isn't correct ??
	return {0, 0, 0};
}

Colour transparentShadingPhong(std::vector<Model> &scene, Light &light, glm::vec3 rayDir, RayTriangleIntersection &intersection, float refractionIndex, int depth, int maxDepth) {
	const float BIAS = 0.01;
	Colour refractionColour{0, 0, 0};

	float u = intersection.u;
	float v = intersection.v;
	ModelTriangle &triangle = intersection.intersectedTriangle;
	glm::vec3 normal = (1 - u - v) * triangle.vertexNormals[0] + u * triangle.vertexNormals[1] + v * triangle.vertexNormals[2];
	normal = glm::normalize(normal);

	// compute fresnel
	float kr = fresnel(rayDir, normal, refractionIndex);
	// int kr = 0;
	bool outside = glm::dot(normal, rayDir) < 0;
	glm::vec3 biasVec = BIAS * normal;
	// compute refraction if it is not a case of total internal reflection
	if (depth < maxDepth) {
		depth += 1;
		if (kr < 1) {
			glm::vec3 refractionDir = glm::normalize(refractRay(rayDir, normal, refractionIndex));
			glm::vec3 refractionRayOrig = outside ? intersection.intersectionPoint - biasVec : intersection.intersectionPoint + biasVec;
			refractionColour = castRay(scene, refractionRayOrig, refractionDir, light, depth, "");
		}

		glm::vec3 reflectionDir = glm::normalize(rayDir - 2.0f * normal * glm::dot(rayDir, normal));
		glm::vec3 reflectionRayOrig = outside ? intersection.intersectionPoint + biasVec : intersection.intersectionPoint - biasVec;
		Colour reflectionColour = castRay(scene, reflectionRayOrig, reflectionDir, light, depth, "");

		// mix the two
		Colour colour;
		colour.red = reflectionColour.red * kr + refractionColour.red * (1 - kr);
		colour.green = reflectionColour.green * kr + refractionColour.green * (1 - kr);
		colour.blue = reflectionColour.blue * kr + refractionColour.blue * (1 - kr);
		return colour;
	}
	return {0, 0, 0};
}

Colour castRay(std::vector<Model> &scene, glm::vec3 origin, glm::vec3 direction, Light &light, int depth, std::string originObjName) {
	float tSmallest = MAXFLOAT;
	RayTriangleIntersection intersection;

	for (Model &model : scene) {
		if (depth > 0 && originObjName != "" && model.name == originObjName) continue;	// don't want to hit ourselves (for mirrors etc.)
		for (int i = 0; i < model.triangles.size(); i++) {
			ModelTriangle &triangle = model.triangles.at(i);
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
		int maxDepth = 10;
		bool inShadow = calculateShadows(scene, light, intersection.intersectionPoint, intersection.intersectedModel.name);
		if (inShadow) {
			switch (intersection.intersectedModel.type) {
				case MIRROR:
					colour = mirror(scene, light, direction, intersection, depth, maxDepth, inShadow);
					break;
				case MIRROR_PHONG:
					colour = mirrorPhong(scene, light, direction, intersection, depth, maxDepth, inShadow);
					break;
				case GLASS: {
					float refractionIndex = 1.5;
					colour = transparentShading(scene, light, direction, intersection, refractionIndex, depth, maxDepth, inShadow);
					break;
				}
				case GLASS_PHONG: {
					float refractionIndex = 1.5;
					colour = transparentShadingPhong(scene, light, direction, intersection, refractionIndex, depth, maxDepth);
					break;
				}
				default:
					colour = ambientLightOnly(light.ambientStrength, intersection);
					break;
			}
		} else {
			switch (intersection.intersectedModel.type) {
				case FLAT:
					colour = diffuseAmbientShading(light, direction, intersection);
					break;
				case FLAT_SPECULAR:
					colour = diffuseAmbientShading(light, direction, intersection);
					colour = specularShading(light, direction, intersection);
					break;
				case SMOOTH_GOURAUD:
					colour = gouraudShading(light, direction, intersection);
					break;
				case SMOOTH_PHONG:
					colour = phongShading(light, direction, intersection);
					break;
				case MIRROR:
					colour = mirror(scene, light, direction, intersection, depth, maxDepth, inShadow);
					break;
				case MIRROR_PHONG:
					colour = mirrorPhong(scene, light, direction, intersection, depth, maxDepth, inShadow);
					break;
				case GLASS: {
					float refractionIndex = 1.5;
					colour = transparentShading(scene, light, direction, intersection, refractionIndex, depth, maxDepth, inShadow);
					break;
				}
				case GLASS_PHONG: {
					float refractionIndex = 1.5;
					colour = transparentShadingPhong(scene, light, direction, intersection, refractionIndex, depth, maxDepth);
					break;
				}
			}
		}
		return colour;
	}
	return {0, 0, 0};
}

void raytrace(DrawingWindow &window, std::vector<Model> &scene, Camera &camera, Light &light) {
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

int main(int argc, char *argv[]) {
	bool rasterising = false;
	bool raytracedOnce = false;
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;

	// load models
	std::vector<Model> scene;
	scene.emplace_back("red-box.obj", "cornell-box.mtl", 0.35, "redBox", FLAT_SPECULAR, true);
	scene.emplace_back("blue-box.obj", "cornell-box.mtl", 0.35, "blueBox", FLAT_SPECULAR, true);
	scene.emplace_back("left-wall.obj", "cornell-box.mtl", 0.35, "leftWall", FLAT_SPECULAR, true);
	scene.emplace_back("right-wall.obj", "cornell-box.mtl", 0.35, "rightWall", FLAT_SPECULAR, true);
	scene.emplace_back("back-wall.obj", "cornell-box.mtl", 0.35, "backWall", FLAT_SPECULAR, true);
	scene.emplace_back("ceiling.obj", "cornell-box.mtl", 0.35, "ceiling", FLAT_SPECULAR, true);
	scene.emplace_back("floor.obj", "cornell-box.mtl", 0.35, "floor", FLAT_SPECULAR, true);
	scene.emplace_back("sphere.obj", "", 0.35, "sphere", GLASS_PHONG, false);

	float focalLength = 4;
	Camera camera{focalLength};
	camera.translate(0, 0, 3);
	camera.rotatePosition(0, 5, 0);
	camera.lookat(0, 0, 0);

	Light light{10, 0.3};
	light.translate(0, 0, 1);
	// light.rotate(0, 10, 0);

	// for (Model& model : scene)
	// 	model.rotate(0, 10, 0);

	while (true) {
		if (rasterising) {
			window.clearPixels();
			rasterise(window, scene, camera);
		} else if (!raytracedOnce) {
			window.clearPixels();
			std::cout << "starting raytracing\n";
			raytrace(window, scene, camera, light);
			raytracedOnce = true;
			std::cout << "finished raytracing\n";
		}
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window, camera, rasterising, raytracedOnce);
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}
