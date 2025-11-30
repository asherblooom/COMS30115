#include "animate.hpp"
#include <functional>
#include <vector>

// transformation will be executed 30 times per second, for a duration of seconds
struct Transformation {
	std::function<void(float, float, float)> transformation;
	float seconds;
};

void doTransformations(std::vector<std::vector<Transformation>>& transformations) {
	return;
}
