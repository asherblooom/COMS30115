#include <CanvasPoint.h>
#include <CanvasTriangle.h>
#include <Colour.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <glm/glm.hpp>
#include "Draw.hpp"
#include "ObjReader.hpp"

CanvasPoint getCanvasIntersectionPoint(glm::vec3 cameraPosition, glm::vec3 vertexPosition, float focalLength) {
	vertexPosition.x *= 500;
	// same reason as below for negative sign
	vertexPosition.y *= -500;
	// blue box has smaller z coord than red box, so need to flip (works because coords are relative to origin (middle) of object)
	vertexPosition.z *= -1;
	// move each vertex so that old origin is now camera position
	vertexPosition += cameraPosition;
	return {(vertexPosition.x * focalLength) / vertexPosition.z + WIDTH / 2.0f, (vertexPosition.y * focalLength) / vertexPosition.z + HEIGHT / 2.0f, vertexPosition.z};
}

CanvasTriangle getCanvasIntersectionTriangle(glm::vec3 cameraPosition, ModelTriangle triangle, float focalLength) {
	CanvasPoint v0 = getCanvasIntersectionPoint(cameraPosition, triangle.vertices[0], focalLength);
	CanvasPoint v1 = getCanvasIntersectionPoint(cameraPosition, triangle.vertices[1], focalLength);
	CanvasPoint v2 = getCanvasIntersectionPoint(cameraPosition, triangle.vertices[2], focalLength);
	return {v0, v1, v2};
}

void handleEvent(SDL_Event event, DrawingWindow &window) {
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_u) {
			Colour colour{std::rand() % 256, std::rand() % 256, std::rand() % 256};
			CanvasTriangle triangle{{float(std::rand() % window.width), float(std::rand() % window.height)},
									{float(std::rand() % window.width), float(std::rand() % window.height)},
									{float(std::rand() % window.width), float(std::rand() % window.height)}};
			drawStokedTriangle(window, triangle, colour);
		} else if (event.key.keysym.sym == SDLK_f) {
			Colour colour{std::rand() % 256, std::rand() % 256, std::rand() % 256};
			CanvasTriangle triangle{{float(std::rand() % window.width), float(std::rand() % window.height)},
									{float(std::rand() % window.width), float(std::rand() % window.height)},
									{float(std::rand() % window.width), float(std::rand() % window.height)}};
			drawFilledTriangle(window, triangle, colour);
		}
	} else if (event.type == SDL_MOUSEBUTTONDOWN) {
		window.savePPM("output.ppm");
		window.saveBMP("output.bmp");
	}
}

int main(int argc, char *argv[]) {
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;

	auto triangles = readObjFile("cornell-box.obj", "cornell-box.mtl", 0.35);
	glm::vec3 cameraPosition = {0, 0, 3};

	while (true) {
		// window.clearPixels();
		// We MUST poll for events - otherwise the window will freeze !
		for (auto &tri : triangles) {
			drawFilledTriangle(window, getCanvasIntersectionTriangle(cameraPosition, tri, 2), tri.colour);
			if (tri.colour.red == 255)
				std::cout << "red: " << tri.vertices[0].z << "\n";
			if (tri.colour.blue == 255)
				std::cout << "blue: " << tri.vertices[0].z << "\n";
			// drawStokedTriangle(window, getCanvasIntersectionTriangle(cameraPosition, tri, 2), tri.colour);
		}
		if (window.pollForInputEvents(event)) handleEvent(event, window);
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}
