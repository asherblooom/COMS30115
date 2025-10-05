#include <CanvasPoint.h>
#include <CanvasTriangle.h>
#include <Colour.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <glm/glm.hpp>
#include "Draw.hpp"
#include "ObjReader.hpp"

#define WIDTH 1920
#define HEIGHT 1060

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

	auto triangles = readObjFile("cornell-box.obj", "cornell-box.mtl", 100);

	while (true) {
		// window.clearPixels();
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window);
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}
