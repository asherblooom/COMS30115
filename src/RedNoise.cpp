#include <CanvasPoint.h>
#include <CanvasTriangle.h>
#include <Colour.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <vector>

#define WIDTH 1920
#define HEIGHT 1060

template <typename T>
std::vector<T> interpolate(T from, T to, int numberOfValues) {
	if (numberOfValues <= 1)
		return std::vector<T>{from};

	T step = (to - from) / float{numberOfValues - 1.0f};
	std::vector<T> values;
	for (int i = 0; i < numberOfValues; i++) {
		values.emplace_back(from);
		from += step;
	}
	return values;
}

void draw(DrawingWindow &window, float x, float y, Colour &colour) {
	float red = colour.red;
	float green = colour.green;
	float blue = colour.blue;
	uint32_t packedColour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
	window.setPixelColour(x, y, packedColour);
}

void drawLine(DrawingWindow &window, CanvasPoint from, CanvasPoint to, Colour colour = {255, 255, 255}) {
	float xDiff = to.x - from.x;
	float yDiff = to.y - from.y;
	float numberOfValues = std::max(std::abs(xDiff), std::abs(yDiff));
	float xStep = xDiff / numberOfValues;
	float yStep = yDiff / numberOfValues;
	for (int i = 0; i < numberOfValues; i++) {
		draw(window, from.x, from.y, colour);
		from.x += xStep;
		from.y += yStep;
	}
}

void drawStokedTriangle(DrawingWindow &window, CanvasTriangle triangle, Colour colour = {255, 255, 255}) {
	drawLine(window, triangle.v0(), triangle.v1(), colour);
	drawLine(window, triangle.v0(), triangle.v2(), colour);
	drawLine(window, triangle.v1(), triangle.v2(), colour);
}

void drawFilledTriangle(DrawingWindow &window, CanvasTriangle triangle, Colour colour = {255, 255, 255}) {
	std::sort(triangle.vertices.begin(), triangle.vertices.end(), [](CanvasPoint &a, CanvasPoint &b) { return a.y < b.y; });
	if (triangle.vertices.at(0).y == triangle.vertices.at(1).y) {
		// interpolate to make lines
		std::vector<float> line1Xs = interpolate(triangle.vertices.at(0).x, triangle.vertices.at(2).x,
												 triangle.vertices.at(2).y - triangle.vertices.at(0).y);
		std::vector<float> line2Xs = interpolate(triangle.vertices.at(1).x, triangle.vertices.at(2).x,
												 triangle.vertices.at(2).y - triangle.vertices.at(1).y);
		for (float y = 0; y < triangle.vertices.at(2).y - triangle.vertices.at(0).y; y++) {
			for (float x = line1Xs.at(y); x <= line2Xs.at(y); x++) {
				draw(window, x, y + triangle.vertices.at(0).y, colour);
			}
		}
	} else if (triangle.vertices.at(1).y == triangle.vertices.at(2).y) {
		// interpolate to make lines
		std::vector<float> line1Xs = interpolate(triangle.vertices.at(0).x, triangle.vertices.at(1).x,
												 triangle.vertices.at(1).y - triangle.vertices.at(0).y);
		std::vector<float> line2Xs = interpolate(triangle.vertices.at(0).x, triangle.vertices.at(2).x,
												 triangle.vertices.at(2).y - triangle.vertices.at(0).y);
		for (float y = 0; y < triangle.vertices.at(2).y - triangle.vertices.at(0).y; y++) {
			for (float x = line1Xs.at(y); x <= line2Xs.at(y); x++) {
				draw(window, x, y + triangle.vertices.at(0).y, colour);
			}
		}
	} else {
		// harder
		// interpolate to make lines
		std::vector<float> line01Xs = interpolate(triangle.vertices.at(0).x, triangle.vertices.at(1).x,
												  triangle.vertices.at(1).y - triangle.vertices.at(0).y);
		std::vector<float> line02Xs = interpolate(triangle.vertices.at(0).x, triangle.vertices.at(2).x,
												  triangle.vertices.at(2).y - triangle.vertices.at(0).y);
		std::vector<float> line12Xs = interpolate(triangle.vertices.at(1).x, triangle.vertices.at(2).x,
												  triangle.vertices.at(2).y - triangle.vertices.at(1).y);
		for (float y = 0; y < triangle.vertices.at(1).y - triangle.vertices.at(0).y; y++) {
			for (float x = line01Xs.at(y); x <= line02Xs.at(y); x++) {
				draw(window, x, y + triangle.vertices.at(0).y, colour);
			}
		}
		// for (float y = 0; y < triangle.vertices.at(2).y - triangle.vertices.at(1).y; y++) {
		// 	for (float x = line12Xs.at(y); x <= line02Xs.at(y + triangle.vertices.at(1).y); x++) {
		// 		draw(window, x, y + triangle.vertices.at(1).y, colour);
		// 	}
		// }
	}

	drawStokedTriangle(window, triangle, {255, 255, 255});
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
	while (true) {
		// window.clearPixels();
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window);
		drawFilledTriangle(window, {{WIDTH / 2.0f, HEIGHT - 100}, {WIDTH - 1, HEIGHT - 100}, {WIDTH / 2.0f + 100, 100}}, {120, 100, 255});
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}
