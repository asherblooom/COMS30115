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
#include "glm/detail/type_mat.hpp"

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
			camVec = Translate(0, 0, -1) * camVec;
		if (event.key.keysym.sym == SDLK_b)
			camVec = Translate(0, 0, 1) * camVec;
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

bool calculateShadows(std::vector<ModelTriangle> triangles, glm::vec3 lightPos, glm::vec3 trianglePos) {
	glm::vec3 shadowRayDir = lightPos - trianglePos;
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
		glm::vec3 tuv = glm::inverse(glm::mat3(-shadowRayDir, e0, e1)) * (trianglePos - v0);
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

void raytrace(DrawingWindow &window, std::vector<ModelTriangle> &triangles, glm::vec4 &camVec, glm::mat4 &camRot, glm::mat4 &modRot, float focalLength) {
	// -------Light Data--------
	glm::vec3 lightPos = {0, 0.5, 0.8};
	float strength = 10;

	// Transform Light Position
	glm::vec4 lightPos4{lightPos, 1};
	lightPos4 = modRot * lightPos4;
	lightPos4 -= camVec;
	lightPos4 = camRot * lightPos4;
	lightPos = {lightPos4.x, lightPos4.y, lightPos4.z};

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
			glm::vec3 camDir = glm::vec3{worldX, worldY, -focalLength} - camVeccy;

			float tSmallest = MAXFLOAT;
			glm::vec3 tSmallestTrianglePos;
			Colour tSmallestColour{0, 0, 0};
			glm::vec3 tSmallestNormal{0, 0, 0};

			for (ModelTriangle &triangle : transformedTriangles) {
				glm::vec3 &v0 = triangle.vertices[0];
				glm::vec3 &v1 = triangle.vertices[1];
				glm::vec3 &v2 = triangle.vertices[2];

				glm::vec3 e0 = v1 - v0;
				glm::vec3 e1 = v2 - v0;
				glm::vec3 tuv = glm::inverse(glm::mat3(-camDir, e0, e1)) * (camVeccy - v0);
				float t = tuv.x;
				float u = tuv.y;
				float v = tuv.z;
				if (t >= 0 && u > 0 && v > 0 && u + v < 1) {
					// we hit this triangle
					if (t < tSmallest) {
						tSmallest = t;
						tSmallestTrianglePos = v0 + e0 * u + e1 * v;
						tSmallestColour = triangle.colour;
						tSmallestNormal = triangle.normal;
					}
				}
			}
			if (tSmallest < MAXFLOAT) {
				// we found a hit in the above loop
				if (calculateShadows(transformedTriangles, lightPos, tSmallestTrianglePos)) {
					tSmallestColour = {0, 0, 0};
				} else {
					// calculate light intensity
					float distance = glm::length(lightPos - tSmallestTrianglePos);
					glm::vec3 direction = glm::normalize(lightPos - tSmallestTrianglePos);

					float intensityAtTri = strength * std::max(glm::dot(direction, tSmallestNormal), 0.0f) / (4 * M_PI * distance);
					float intensityCapped = intensityAtTri < 1 ? intensityAtTri : 1;
					tSmallestColour.red *= intensityCapped;
					tSmallestColour.blue *= intensityCapped;
					tSmallestColour.green *= intensityCapped;
				}
			}
			draw(window, w, h, tSmallestColour);
		}
	}
}

int main(int argc, char *argv[]) {
	bool rasterising = false;
	bool raytracedOnce = false;
	float focalLength = 3;
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;

	std::vector<ModelTriangle> triangles = readObjFile("cornell-box.obj", "cornell-box.mtl", 0.35);
	for (auto &triangle : triangles) {
		triangle.normal = glm::normalize(glm::cross(triangle.vertices[1] - triangle.vertices[0], triangle.vertices[2] - triangle.vertices[0]));
	}

	glm::vec4 camVec = Translate(0, 0, 2) * glm::vec4(0, 0, 0, 1);
	glm::mat4 camRot{1};
	glm::mat4 modRot{1};

	while (true) {
		if (rasterising) {
			window.clearPixels();
			rasterise(window, triangles, camVec, camRot, modRot, focalLength);
		} else if (!raytracedOnce) {
			window.clearPixels();
			raytrace(window, triangles, camVec, camRot, modRot, focalLength);
			raytracedOnce = true;
		}
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window, camVec, camRot, modRot, rasterising, raytracedOnce);
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}
