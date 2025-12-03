#include "Draw.hpp"
#include <TexturePoint.h>
#include <Utils.h>
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <vector>

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

void drawLine(DrawingWindow &window, CanvasPoint from, CanvasPoint to, Colour colour) {
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

void drawBaryTriangle(DrawingWindow &window, CanvasTriangle triangle) {
	std::sort(triangle.vertices.begin(), triangle.vertices.end(), [](CanvasPoint &a, CanvasPoint &b) { return a.y < b.y; });
	CanvasPoint &v0 = triangle.vertices.at(0);	// vertex with lowest y value
	CanvasPoint &v1 = triangle.vertices.at(1);	// vertex with mid y value
	CanvasPoint &v2 = triangle.vertices.at(2);	// vertex with highest y value
	std::vector<float> xs1, xs2;
	if (v0.y == v1.y) {
		xs1 = interpolate(v0.x, v2.x, v2.y - v0.y);
		xs2 = interpolate(v1.x, v2.x, v2.y - v1.y);
	} else if (v1.y == v2.y) {
		xs1 = interpolate(v0.x, v1.x, v1.y - v0.y);
		xs2 = interpolate(v0.x, v2.x, v2.y - v0.y);
	}

	// these are pointers to avoid copying
	std::vector<float> *leftXs;
	std::vector<float> *rightXs;
	float minHalfway = std::min(xs1.size() / 2, xs2.size() / 2);
	if (xs1.at(minHalfway) <= xs2.at(minHalfway)) {
		leftXs = &xs1;
		rightXs = &xs2;
	} else {
		leftXs = &xs2;
		rightXs = &xs1;
	}
	for (int y = v0.y; y < v2.y; y++) {
		for (float x = leftXs->at(y - v0.y); x <= rightXs->at(y - v0.y); x++) {
			glm::vec3 bary = convertToBarycentricCoordinates({v0.x, v0.y}, {v1.x, v1.y}, {v2.x, v2.y}, {x, y});
			Colour baryColour{(int)(bary.x * 255), (int)(bary.z * 255), (int)(bary.y * 255)};
			draw(window, x, y, baryColour);
		}
	}
}

void drawStokedTriangle(DrawingWindow &window, CanvasTriangle triangle, Colour colour) {
	drawLine(window, triangle.v0(), triangle.v1(), colour);
	drawLine(window, triangle.v0(), triangle.v2(), colour);
	drawLine(window, triangle.v1(), triangle.v2(), colour);
}

void fillFlatTriangle(DrawingWindow &window, std::vector<float> &depthBuffer,
					  int yStart, int yEnd,
					  std::vector<float> &xs1, std::vector<float> &xs2,
					  std::vector<float> &zs1, std::vector<float> zs2,
					  Colour &colour) {
	// these are pointers to avoid copying
	std::vector<float> *leftXs;
	std::vector<float> *rightXs;
	std::vector<float> *leftZs;
	std::vector<float> *rightZs;
	// for triangle to be valid, the lines (the xs) cannot intersect other than at positions 0 and size - 1
	// so to check which side each is on, we take the min halfway point and use that
	// this also assumes the zs and xs match up
	float minHalfway = std::min(xs1.size() / 2, xs2.size() / 2);
	if (xs1.at(minHalfway) <= xs2.at(minHalfway)) {
		leftXs = &xs1;
		rightXs = &xs2;
		leftZs = &zs1;
		rightZs = &zs2;
	} else {
		leftXs = &xs2;
		rightXs = &xs1;
		leftZs = &zs2;
		rightZs = &zs1;
	}
	for (int y = 0; y <= yEnd - yStart; y++) {
		int xStart = std::round(leftXs->at(y));
		int xEnd = std::round(rightXs->at(y));
		std::vector<float> horizontalZs = interpolate(1 / leftZs->at(y), 1 / rightZs->at(y), xEnd - xStart + 1);
		for (float x = 0; x <= xEnd - xStart; x++) {
			if (depthBuffer[WIDTH * (y + yStart) + (x + xStart)] < horizontalZs.at(x)) {
				draw(window, x + xStart, y + yStart, colour);
				depthBuffer[WIDTH * (y + yStart) + (x + xStart)] = horizontalZs.at(x);
			}
		}
	}
}

void drawFilledTriangle(DrawingWindow &window, std::vector<float> &depthBuffer, CanvasTriangle triangle, Colour colour) {
	for (CanvasPoint &v : triangle.vertices) {
		if (v.x < 0) v.x = 0;
		if (v.x >= WIDTH) v.x = WIDTH - 1;
		if (v.y < 0) v.y = 0;
		if (v.y >= HEIGHT) v.y = HEIGHT - 1;
	}
	std::sort(triangle.vertices.begin(), triangle.vertices.end(), [](CanvasPoint &a, CanvasPoint &b) { return a.y < b.y; });
	CanvasPoint &v0 = triangle.vertices.at(0);	// vertex with lowest y value
	CanvasPoint &v1 = triangle.vertices.at(1);	// vertex with mid y value
	CanvasPoint &v2 = triangle.vertices.at(2);	// vertex with highest y value
	// interpolate to make vectors of all x-coords in each line
	std::vector<float> line01xs = interpolate(v0.x, v1.x, std::ceil(v1.y - v0.y) + 1);
	std::vector<float> line02xs = interpolate(v0.x, v2.x, std::ceil(v2.y - v0.y) + 1);
	std::vector<float> line12xs = interpolate(v1.x, v2.x, std::ceil(v2.y - v1.y) + 1);

	std::vector<float> line01zs = interpolate(v0.depth, v1.depth, std::ceil(v1.y - v0.y) + 1);
	std::vector<float> line02zs = interpolate(v0.depth, v2.depth, std::ceil(v2.y - v0.y) + 1);
	std::vector<float> line12zs = interpolate(v1.depth, v2.depth, std::ceil(v2.y - v1.y) + 1);

	if (v0.y == v1.y) {
		fillFlatTriangle(window, depthBuffer, v0.y, v2.y, line02xs, line12xs, line02zs, line12zs, colour);
	} else if (v1.y == v2.y) {
		fillFlatTriangle(window, depthBuffer, v0.y, v2.y, line01xs, line02xs, line01zs, line02zs, colour);
	} else {
		// need to fill triangle in 2 goes
		// the first x value we want to use must be at position 0, so we have to shorten front of vector for bottom part of line
		std::vector<float> bottomLine02xs{line02xs.begin() + (v1.y - v0.y), line02xs.end()};
		std::vector<float> bottomLine02zs{line02zs.begin() + (v1.y - v0.y), line02zs.end()};
		fillFlatTriangle(window, depthBuffer, v0.y, v1.y, line01xs, line02xs, line01zs, line02zs, colour);
		fillFlatTriangle(window, depthBuffer, v1.y, v2.y, line12xs, bottomLine02xs, line12zs, bottomLine02zs, colour);
	}
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

	for (int y = 0; y <= yEnd - yStart; y++) {
		int xStart = (int)leftXs->at(y);
		int xEnd = (int)rightXs->at(y);
		std::vector<glm::vec2> texLine = interpolate(leftTexLine->at(y), rightTexLine->at(y), xEnd - xStart + 1);
		for (int x = 0; x <= xEnd - xStart; x++) {
			glm::vec2 texPos = texLine.at(x);
			uint32_t colour = tex.pixels.at((int)texPos.y * tex.width + (int)texPos.x);
			window.setPixelColour(x + xStart, y + yStart, colour);
		}
	}
}

void drawTexturedTriangle(DrawingWindow &window, CanvasTriangle triangle, TextureMap texture) {
	std::sort(triangle.vertices.begin(), triangle.vertices.end(), [](CanvasPoint &a, CanvasPoint &b) { return a.y < b.y; });
	CanvasPoint &v0 = triangle.vertices[0];	 // vertex with lowest y value
	CanvasPoint &v1 = triangle.vertices[1];	 // vertex with mid y value
	CanvasPoint &v2 = triangle.vertices[2];	 // vertex with highest y value
	TexturePoint &t0 = v0.texturePoint;
	TexturePoint &t1 = v1.texturePoint;
	TexturePoint &t2 = v2.texturePoint;

	if (t0.x < 0 || t0.x > texture.width || t1.x < 0 || t1.x > texture.width || t2.x < 0 || t2.x > texture.width || t0.y < 0 || t2.y > texture.height) {
		std::cerr << "ERROR: texture points are outside of texture\n";
		return;
	}

	// interpolate to make vectors of all x-coords in each line
	std::vector<float> line01xs = interpolate(v0.x, v1.x, std::ceil(v1.y - v0.y) + 1);
	std::vector<float> line02xs = interpolate(v0.x, v2.x, std::ceil(v2.y - v0.y) + 1);
	std::vector<float> line12xs = interpolate(v1.x, v2.x, std::ceil(v2.y - v1.y) + 1);

	std::vector<glm::vec2> texLine01 = interpolate(glm::vec2{t0.x, t0.y}, glm::vec2{t1.x, t1.y}, std::ceil(v1.y - v0.y) + 1);
	std::vector<glm::vec2> texLine02 = interpolate(glm::vec2{t0.x, t0.y}, glm::vec2{t2.x, t2.y}, std::ceil(v2.y - v0.y) + 1);
	std::vector<glm::vec2> texLine12 = interpolate(glm::vec2{t1.x, t1.y}, glm::vec2{t2.x, t2.y}, std::ceil(v2.y - v1.y) + 1);

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

	drawStokedTriangle(window, triangle, {255, 255, 255});
}
