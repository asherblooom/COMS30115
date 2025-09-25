#include <CanvasPoint.h>
#include <CanvasTriangle.h>
#include <Colour.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <vector>
#include "TextureMap.h"
#include "TexturePoint.h"

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

void fillFlatTriangle(DrawingWindow &window, int yStart, int yEnd, std::vector<float> &xs1, std::vector<float> &xs2, Colour &colour) {
	// these are pointers to avoid copying
	std::vector<float> *leftXs;
	std::vector<float> *rightXs;
	// for triangle to be valid, the lines (the xs) cannot intersect other than at positions 0 and size - 1
	// so to check which side each is on, we take the min halfway point and use that
	float minHalfway = std::min(xs1.size() / 2, xs2.size() / 2);
	if (xs1.at(minHalfway) <= xs2.at(minHalfway)) {
		leftXs = &xs1;
		rightXs = &xs2;
	} else {
		leftXs = &xs2;
		rightXs = &xs1;
	}
	for (int y = yStart; y < yEnd; y++) {
		for (float x = leftXs->at(y - yStart); x <= rightXs->at(y - yStart); x++) {
			draw(window, x, y, colour);
		}
	}
}

void drawFilledTriangle(DrawingWindow &window, CanvasTriangle triangle, Colour colour = {255, 255, 255}) {
	std::sort(triangle.vertices.begin(), triangle.vertices.end(), [](CanvasPoint &a, CanvasPoint &b) { return a.y < b.y; });
	CanvasPoint &v0 = triangle.vertices.at(0);	// vertex with lowest y value
	CanvasPoint &v1 = triangle.vertices.at(1);	// vertex with mid y value
	CanvasPoint &v2 = triangle.vertices.at(2);	// vertex with highest y value
	// interpolate to make vectors of all x-coords in each line
	std::vector<float> line01xs = interpolate(v0.x, v1.x, v1.y - v0.y);
	std::vector<float> line02xs = interpolate(v0.x, v2.x, v2.y - v0.y);
	std::vector<float> line12xs = interpolate(v1.x, v2.x, v2.y - v1.y);

	if (v0.y == v1.y) {
		fillFlatTriangle(window, v0.y, v2.y, line02xs, line12xs, colour);
	} else if (v1.y == v2.y) {
		fillFlatTriangle(window, v0.y, v2.y, line01xs, line02xs, colour);
	} else {
		// need to fill triangle in 2 goes
		// the first x value we want to use must be at position 0, so we have to shorten front of vector for bottom part of line
		std::vector<float> bottomLine02xs{line02xs.begin() + (v1.y - v0.y), line02xs.end()};
		fillFlatTriangle(window, v0.y, v1.y, line01xs, line02xs, colour);
		fillFlatTriangle(window, v1.y, v2.y, line12xs, bottomLine02xs, colour);
	}

	drawStokedTriangle(window, triangle, {255, 255, 255});
}

void textureFlatTriangle(DrawingWindow &window, TextureMap &tex, int yStart, int yEnd,
						 std::vector<float> &xs1, std::vector<float> &xs2,
						 std::vector<glm::vec2> &texLine1, std::vector<glm::vec2> &texLine2) {
	// these are pointers to avoid copying
	std::vector<float> *leftXs;
	std::vector<float> *rightXs;
	std::vector<glm::vec2> *leftTexLine;
	std::vector<glm::vec2> *rightTexLine;
	// for triangle to be valid, the lines (the xs) cannot intersect other than at positions 0 and size - 1
	// so to check which side each is on, we take the min halfway point and use that
	float minHalfway = std::min(xs1.size() / 2, xs2.size() / 2);
	if (xs1.at(minHalfway) <= xs2.at(minHalfway)) {
		leftXs = &xs1;
		rightXs = &xs2;
	} else {
		leftXs = &xs2;
		rightXs = &xs1;
	}

	if (texLine1.at(minHalfway).x <= texLine2.at(minHalfway).x) {
		leftTexLine = &texLine1;
		rightTexLine = &texLine2;
	} else {
		leftTexLine = &texLine2;
		rightTexLine = &texLine1;
	}

	for (int y = yStart; y < yEnd; y++) {
		float xStart = leftXs->at(y - yStart);
		float xEnd = rightXs->at(y - yStart);
		std::vector<glm::vec2> texLine = interpolate(leftTexLine->at(y), rightTexLine->at(y), xEnd - xStart);
		for (float x = xStart; x <= xEnd; x++) {
			glm::vec2 texPos = texLine.at(x);
			window.setPixelColour(x, y, tex.pixels.at(texPos.y * tex.width + texPos.x));
		}
	}
}

void drawTexturedTriangle(DrawingWindow &window, CanvasTriangle triangle, TextureMap texture) {
	std::sort(triangle.vertices.begin(), triangle.vertices.end(), [](CanvasPoint &a, CanvasPoint &b) { return a.y < b.y; });
	CanvasPoint &v0 = triangle.vertices.at(0);	// vertex with lowest y value
	CanvasPoint &v1 = triangle.vertices.at(1);	// vertex with mid y value
	CanvasPoint &v2 = triangle.vertices.at(2);	// vertex with highest y value

	TexturePoint &t0 = v0.texturePoint;
	TexturePoint &t1 = v1.texturePoint;
	TexturePoint &t2 = v2.texturePoint;

	// interpolate to make vectors of all x-coords in each line
	std::vector<float> line01xs = interpolate(v0.x, v1.x, v1.y - v0.y);
	std::vector<float> line02xs = interpolate(v0.x, v2.x, v2.y - v0.y);
	std::vector<float> line12xs = interpolate(v1.x, v2.x, v2.y - v1.y);

	std::vector<glm::vec2> texLine01 = interpolate(glm::vec2{t0.x, t0.y}, glm::vec2{t1.x, t1.y}, t1.y - t0.y);
	std::vector<glm::vec2> texLine02 = interpolate(glm::vec2{t0.x, t0.y}, glm::vec2{t2.x, t2.y}, t2.y - t0.y);
	std::vector<glm::vec2> texLine12 = interpolate(glm::vec2{t1.x, t1.y}, glm::vec2{t2.x, t2.y}, t2.y - t1.y);

	if (v0.y == v1.y) {
		textureFlatTriangle(window, texture, v0.y, v2.y, line02xs, line12xs, texLine02, texLine12);
	} else if (v1.y == v2.y) {
		textureFlatTriangle(window, texture, v0.y, v2.y, line01xs, line02xs, texLine01, texLine02);
	} else {
		// need to fill triangle in 2 goes
		// the first x value we want to use must be at position 0, so we have to shorten front of vector for bottom part of line
		std::vector<float> bottomLine02xs{line02xs.begin() + (v1.y - v0.y), line02xs.end()};
		std::vector<glm::vec2> bottomTexLine02{texLine02.begin() + (v1.y - v0.y), texLine02.end()};
		textureFlatTriangle(window, texture, v0.y, v1.y, line01xs, line02xs, texLine01, texLine02);
		textureFlatTriangle(window, texture, v1.y, v2.y, line12xs, bottomLine02xs, texLine12, bottomTexLine02);
	}
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
		// drawFilledTriangle(window, {{WIDTH / 2.0f, HEIGHT - 100}, {WIDTH - 1, HEIGHT - 100}, {WIDTH / 2.0f + 100, 100}}, {120, 100, 255});
		drawTexturedTriangle(window, {{WIDTH / 2.0f, HEIGHT - 100}, {WIDTH - 1, HEIGHT - 100}, {WIDTH / 2.0f + 100, 100}}, {"texture.ppm"});
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}
