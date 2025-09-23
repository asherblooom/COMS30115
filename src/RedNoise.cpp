#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <glm/glm.hpp>
#include <vector>
#include "glm/detail/func_vector_relational.hpp"

#define WIDTH 320
#define HEIGHT 240

std::vector<float> interpolateSingleFloats(float from, float to, int numberOfValues) {
	float interval = (to - from) / (numberOfValues - 1);
	std::vector<float> values;
	// (too?) simple fix that should allow us to compare floats
	float tolerance = 0.01;
	for (float n = from; !(n <= to + tolerance && n >= to - tolerance); n += interval)
		values.emplace_back(n);
	values.emplace_back(to);  // dodgy fix...... adds the final value (assumes it is equal to values.back()+interval)
	return values;
}

std::vector<glm::vec3> interpolateThreeElementValues(glm::vec3 from, glm::vec3 to, int numberOfValues) {
	glm::vec3 interval = (to - from) / glm::vec3(numberOfValues - 1);
	std::vector<glm::vec3> values;
	glm::vec3 tolerance(0.01);
	for (glm::vec3 n = from; !(glm::all(glm::lessThanEqual(n, to + tolerance)) && glm::all(glm::greaterThanEqual(n, to - tolerance))); n += interval)
		values.emplace_back(n);
	values.emplace_back(to);
	return values;
}

void draw(DrawingWindow &window, std::vector<glm::vec3> &leftColumn, std::vector<glm::vec3> &rightColumn) {
	window.clearPixels();
	for (size_t y = 0; y < window.height; y++) {
		glm::vec3 leftPoint = leftColumn.at(y);
		glm::vec3 rightPoint = rightColumn.at(y);
		std::vector<glm::vec3> colours = interpolateThreeElementValues(leftPoint, rightPoint, window.width);
		for (size_t x = 0; x < window.width; x++) {
			float red = colours.at(x).r;
			float green = colours.at(x).g;
			float blue = colours.at(x).b;
			uint32_t colour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
			window.setPixelColour(x, y, colour);
		}
	}
}

void handleEvent(SDL_Event event, DrawingWindow &window) {
	// clang-format off
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_LEFT) std::cout << "LEFT" << std::endl;
		else if (event.key.keysym.sym == SDLK_RIGHT) std::cout << "RIGHT" << std::endl;
		else if (event.key.keysym.sym == SDLK_UP) std::cout << "UP" << std::endl;
		else if (event.key.keysym.sym == SDLK_DOWN) std::cout << "DOWN" << std::endl;
	} else if (event.type == SDL_MOUSEBUTTONDOWN) {
		window.savePPM("output.ppm");
		window.saveBMP("output.bmp");
	}
	// clang-format on
}

int main(int argc, char *argv[]) {
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;
	glm::vec3 from(1, 4, 9.2);
	glm::vec3 to(4, 1, 9.8);
	auto result = interpolateThreeElementValues(from, to, 4);
	std::cout << "size: " << result.size() << "\n";
	for (auto vec : result) {
		std::cout << "(" << vec.x << ", " << vec.y << ", " << vec.z << ")\n";
	}
	glm::vec3 topLeft(255, 0, 0);		// red
	glm::vec3 topRight(0, 0, 255);		// blue
	glm::vec3 bottomRight(0, 255, 0);	// green
	glm::vec3 bottomLeft(255, 255, 0);	// yellow

	std::vector<glm::vec3> leftColumn = interpolateThreeElementValues(topLeft, bottomLeft, window.height);
	std::vector<glm::vec3> rightColumn = interpolateThreeElementValues(topRight, bottomRight, window.height);

	std::vector<float> vals = interpolateSingleFloats(255, 0, window.width);
	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window);
		draw(window, leftColumn, rightColumn);
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}
