#include <CanvasPoint.h>
#include <CanvasTriangle.h>
#include <Colour.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include "Draw.hpp"
#include "ObjReader.hpp"
#include "Transform.hpp"
#include "glm/gtc/quaternion.hpp"

CanvasPoint getCanvasIntersectionPoint(glm::vec4 &cameraPosition, glm::mat4 &cameraRotation, glm::mat4 &modelRotation, glm::vec3 &vertexPosition, float focalLength) {
	glm::vec4 vertexPos{vertexPosition, 1};

	vertexPos = modelRotation * vertexPos;
	vertexPos -= cameraPosition;
	vertexPos = cameraRotation * vertexPos;

	vertexPos += cameraPosition;
	// FIXME: ask TA's about scaling model and funny behaviour when scale = 100, also about black lines appearing when rotating etc
	// also about boxes going through floor - is it ok??
	vertexPos.x *= 100;
	// same reason as below for negative sign
	vertexPos.y *= -100;
	// blue box has smaller z coord than red box, so need to flip (works because coords are relative to origin (middle) of object)
	vertexPos -= cameraPosition;
	vertexPos.z *= -1;
	return {(vertexPos.x * focalLength) / vertexPos.z + WIDTH / 2.0f, (vertexPos.y * focalLength) / vertexPos.z + HEIGHT / 2.0f, vertexPos.z};
}

CanvasTriangle getCanvasIntersectionTriangle(glm::vec4 &cameraPosition, glm::mat4 &cameraRotation, glm::mat4 &modelRotation, ModelTriangle &triangle, float focalLength) {
	CanvasPoint v0 = getCanvasIntersectionPoint(cameraPosition, cameraRotation, modelRotation, triangle.vertices[0], focalLength);
	CanvasPoint v1 = getCanvasIntersectionPoint(cameraPosition, cameraRotation, modelRotation, triangle.vertices[1], focalLength);
	CanvasPoint v2 = getCanvasIntersectionPoint(cameraPosition, cameraRotation, modelRotation, triangle.vertices[2], focalLength);
	return {v0, v1, v2};
}

void handleEvent(SDL_Event event, DrawingWindow &window, glm::vec4 &camVec, glm::mat4 &camRot, glm::mat4 &modRot) {
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_LEFT)
			camVec = Translate(10, 0, 0) * camVec;
		if (event.key.keysym.sym == SDLK_RIGHT)
			camVec = Translate(-10, 0, 0) * camVec;
		if (event.key.keysym.sym == SDLK_UP)
			camVec = Translate(0, 10, 0) * camVec;
		if (event.key.keysym.sym == SDLK_DOWN)
			camVec = Translate(0, -10, 0) * camVec;
		if (event.key.keysym.sym == SDLK_f)
			camVec = Translate(0, 0, -2) * camVec;
		if (event.key.keysym.sym == SDLK_b)
			camVec = Translate(0, 0, 2) * camVec;
		// cam rot
		if (event.key.keysym.sym == SDLK_w)
			camRot = Rotate(2, 0, 0) * camRot;
		if (event.key.keysym.sym == SDLK_s)
			camRot = Rotate(-2, 0, 0) * camRot;
		if (event.key.keysym.sym == SDLK_a)
			camRot = Rotate(0, -2, 0) * camRot;
		if (event.key.keysym.sym == SDLK_d)
			camRot = Rotate(0, 2, 0) * camRot;
		if (event.key.keysym.sym == SDLK_o)
			camRot = Rotate(0, 0, 2) * camRot;
		if (event.key.keysym.sym == SDLK_p)
			camRot = Rotate(0, 0, -2) * camRot;
		// mod rot
		if (event.key.keysym.sym == SDLK_l)
			modRot = Rotate(0, -2, 0) * modRot;
		if (event.key.keysym.sym == SDLK_h)
			modRot = Rotate(0, 2, 0) * modRot;
		if (event.key.keysym.sym == SDLK_k)
			modRot = Rotate(2, 0, 0) * modRot;
		if (event.key.keysym.sym == SDLK_j)
			modRot = Rotate(-2, 0, 0) * modRot;
	} else if (event.type == SDL_MOUSEBUTTONDOWN) {
		window.savePPM("output.ppm");
		window.saveBMP("output.bmp");
	}
}

void rasterise(DrawingWindow &window, std::vector<ModelTriangle> &triangles, glm::vec4 &camVec, glm::mat4 &camRot, glm::mat4 &modRot) {
	// reset depth buffer
	std::vector<float> depthBuffer((window.width * window.height), 0);
	for (ModelTriangle &triangle : triangles) {
		CanvasTriangle canvasTri = getCanvasIntersectionTriangle(camVec, camRot, modRot, triangle, 2);
		drawFilledTriangle(window, depthBuffer, canvasTri, triangle.colour);
	}
}

bool calculateShadows(std::vector<ModelTriangle> triangles, glm::vec3 lightPos, glm::vec3 trianglePos) {
	glm::vec3 shadowRayDir = lightPos - trianglePos;
	float distToLight = glm::length(shadowRayDir);

	bool needShadow = false;

	for (ModelTriangle &triangle : triangles) {
		// scale verticies
		glm::vec3 v0 = triangle.vertices[0] * glm::vec3{100, -100, 1};
		glm::vec3 v1 = triangle.vertices[1] * glm::vec3{100, -100, 1};
		glm::vec3 v2 = triangle.vertices[2] * glm::vec3{100, -100, 1};

		glm::vec3 e0 = v1 - v0;
		glm::vec3 e1 = v2 - v0;
		glm::vec3 tuv = glm::inverse(glm::mat3(-shadowRayDir, e0, e1)) * (trianglePos - v0);
		float t = tuv.x;
		float u = tuv.y;
		float v = tuv.z;
		if (t >= 0 && u > 0 && v > 0 && u + v < 1) {
			// we hit this triangle
			// t > 0.00001 prevents shadow achne
			if (t < distToLight && t > 0.00001) {
				needShadow = true;
				break;
			}
		}
	}
	return needShadow;
}

void raytrace(DrawingWindow &window, std::vector<ModelTriangle> &triangles, glm::vec3 &camVec, glm::mat4 &camRot, glm::mat4 &modRot) {
	for (int h = 0; h < window.height; h++) {
		for (int w = 0; w < window.width; w++) {
			glm::vec3 camDir = glm::vec3{w - window.width / 2.0f, h - window.height / 2.0f, 0} - camVec;
			float tSmallest = MAXFLOAT;
			glm::vec3 tSmallestTrianglePos;
			Colour tSmallestColour{0, 0, 0};

			for (ModelTriangle &triangle : triangles) {
				// scale verticies
				glm::vec3 v0 = triangle.vertices[0] * glm::vec3{100, -100, 1};
				glm::vec3 v1 = triangle.vertices[1] * glm::vec3{100, -100, 1};
				glm::vec3 v2 = triangle.vertices[2] * glm::vec3{100, -100, 1};

				glm::vec3 e0 = v1 - v0;
				glm::vec3 e1 = v2 - v0;
				glm::vec3 tuv = glm::inverse(glm::mat3(-camDir, e0, e1)) * (camVec - v0);
				float t = tuv.x;
				float u = tuv.y;
				float v = tuv.z;
				if (t >= 0 && u > 0 && v > 0 && u + v < 1) {
					// we hit this triangle
					if (t < tSmallest) {
						tSmallest = t;
						tSmallestTrianglePos = v0 + e0 * u + e1 * v;
						tSmallestColour = triangle.colour;
					}
				}
			}
			if (tSmallest < MAXFLOAT) {
				// we found a hit in the above loop
				glm::vec3 lightPos = {0, -80, 1};
				if (calculateShadows(triangles, lightPos, tSmallestTrianglePos)) {
					tSmallestColour = {0, 0, 0};
				}
			}
			draw(window, w, h, tSmallestColour);
		}
	}
}

int main(int argc, char *argv[]) {
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;

	auto triangles = readObjFile("cornell-box.obj", "cornell-box.mtl", 0.35);
	glm::vec4 camVec = Translate(0, 0, 3) * glm::vec4(0, 0, 0, 1);
	glm::mat4 camRot{1};
	glm::mat4 modRot{1};

	glm::vec3 camVeccy{camVec.x, camVec.y, camVec.z};
	raytrace(window, triangles, camVeccy, camRot, modRot);
	while (true) {
		// window.clearPixels();
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window, camVec, camRot, modRot);
		// rasterise(window, triangles, camVec, camRot, modRot);
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}
