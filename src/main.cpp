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

CanvasPoint getCanvasIntersectionPoint(glm::vec4 &cameraPosition, glm::mat4 &cameraRotation, glm::vec3 &vertexPosition, float focalLength) {
	glm::vec4 vertexPos{vertexPosition, 1};
	vertexPos.x *= 500;
	// same reason as below for negative sign
	vertexPos.y *= -500;
	// blue box has smaller z coord than red box, so need to flip (works because coords are relative to origin (middle) of object)
	vertexPos.z *= -1;
	// move each vertex so that old origin is now camera position
	vertexPos += cameraPosition;
	vertexPos = glm::inverse(cameraRotation) * vertexPos;
	return {(vertexPos.x * focalLength) / vertexPos.z + WIDTH / 2.0f, (vertexPos.y * focalLength) / vertexPos.z + HEIGHT / 2.0f, vertexPos.z};
}

CanvasTriangle getCanvasIntersectionTriangle(glm::vec4 &cameraPosition, glm::mat4 &cameraRotation, ModelTriangle &triangle, float focalLength) {
	CanvasPoint v0 = getCanvasIntersectionPoint(cameraPosition, cameraRotation, triangle.vertices[0], focalLength);
	CanvasPoint v1 = getCanvasIntersectionPoint(cameraPosition, cameraRotation, triangle.vertices[1], focalLength);
	CanvasPoint v2 = getCanvasIntersectionPoint(cameraPosition, cameraRotation, triangle.vertices[2], focalLength);
	return {v0, v1, v2};
}

void handleEvent(SDL_Event event, DrawingWindow &window, glm::vec4 &camVec, glm::mat4 &camRot) {
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_LEFT)
			camVec = Translate(-10, 0, 0) * camVec;
		if (event.key.keysym.sym == SDLK_RIGHT)
			camVec = Translate(10, 0, 0) * camVec;
		if (event.key.keysym.sym == SDLK_UP)
			camVec = Translate(0, -10, 0) * camVec;
		if (event.key.keysym.sym == SDLK_DOWN)
			camVec = Translate(0, 10, 0) * camVec;
		if (event.key.keysym.sym == SDLK_f)
			camVec = Translate(0, 0, -2) * camVec;
		if (event.key.keysym.sym == SDLK_b)
			camVec = Translate(0, 0, 2) * camVec;
		if (event.key.keysym.sym == SDLK_w)
			camRot = Rotate(0.3, 0, 0) * camRot;
		if (event.key.keysym.sym == SDLK_s)
			camRot = Rotate(-0.3, 0, 0) * camRot;
		if (event.key.keysym.sym == SDLK_a)
			camRot = Rotate(0, -0.3, 0) * camRot;
		if (event.key.keysym.sym == SDLK_d)
			camRot = Rotate(0, 0.3, 0) * camRot;
		if (event.key.keysym.sym == SDLK_o)
			camRot = Rotate(0, 0, 0.3) * camRot;
		if (event.key.keysym.sym == SDLK_p)
			camRot = Rotate(0, 0, -0.3) * camRot;
	} else if (event.type == SDL_MOUSEBUTTONDOWN) {
		window.savePPM("output.ppm");
		window.saveBMP("output.bmp");
	}
}

int main(int argc, char *argv[]) {
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;

	auto triangles = readObjFile("cornell-box.obj", "cornell-box.mtl", 0.35);
	glm::vec4 camVec = Translate(0, 0, 3) * glm::vec4(0, 0, 0, 1);
	glm::mat4 camRot{1};

	while (true) {
		window.clearPixels();
		// reset depth buffer
		std::vector<float> depthBuffer((WIDTH * HEIGHT), 0);
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window, camVec, camRot);
		for (auto &tri : triangles) {
			CanvasTriangle canvasTri = getCanvasIntersectionTriangle(camVec, camRot, tri, 2);
			drawFilledTriangle(window, depthBuffer, canvasTri, tri.colour);
		}
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}
