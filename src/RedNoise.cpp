#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <glm/glm.hpp>
#include <vector>

#define WIDTH 320
#define HEIGHT 240

template <typename T>
std::vector<T> interpolate(T from, T to, int numberOfValues) {
	if (numberOfValues <= 1)
		return std::vector<T>{from};

	T interval = (to - from) / float{numberOfValues - 1.0f};
	T currValue{from};
	std::vector<T> values;

	for (int i = 0; i < numberOfValues; i++) {
		values.emplace_back(currValue);
		currValue += interval;
	}
	return values;
}

void draw(DrawingWindow &window, std::vector<glm::vec3> &leftColumn, std::vector<glm::vec3> &rightColumn) {
	window.clearPixels();
	for (size_t y = 0; y < window.height; y++) {
		glm::vec3 leftPoint = leftColumn.at(y);
		glm::vec3 rightPoint = rightColumn.at(y);
		std::vector<glm::vec3> colours = interpolate(leftPoint, rightPoint, window.width);
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

	glm::vec3 topLeft(255, 0, 0);		// red
	glm::vec3 topRight(0, 0, 255);		// blue
	glm::vec3 bottomRight(0, 255, 0);	// green
	glm::vec3 bottomLeft(255, 255, 0);	// yellow
	std::vector<glm::vec3> leftColumn = interpolate(topLeft, bottomLeft, window.height);
	std::vector<glm::vec3> rightColumn = interpolate(topRight, bottomRight, window.height);

	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window);
		draw(window, leftColumn, rightColumn);
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}
