void drawFilledTriangle(CanvasTriangle triangle, Colour colour, DrawingWindow &window, float depthBuffer[WIDTH][HEIGHT]) {
	// order points by y value - point1 has lowest (highest point in window due to way the co-ords work)
	CanvasPoint point1 = triangle.v0();
	CanvasPoint point2 = triangle.v1();
	CanvasPoint point3 = triangle.v2();

	if (point1.y > point2.y) {
		std::swap(point1, point2);
	}
	if (point2.y > point3.y) {
		std::swap(point2, point3);
	}
	if (point1.y > point2.y) {
		std::swap(point1, point2);
	}

	// handle trivial case - horizontal line
	if (point1.y == point3.y) {
		float minX = std::min(point1.x, std::min(point2.x, point3.x));
		float maxX = std::max(point1.x, std::max(point2.x, point3.x));
		drawLine(CanvasPoint(minX, point1.y), CanvasPoint(maxX, point1.y), colour, window);
		return;
	}

	// interpolate the x value for the point level with point 2 (by y value) on the line from point1 to point3
	float t = (point2.y - point1.y) / (point3.y - point1.y);
	float levelPointX = point1.x + (t * (point3.x - point1.x));
	float levelPointDepth = point1.depth + (t * (point3.depth - point1.depth));
	CanvasPoint levelPoint = {levelPointX, point2.y, levelPointDepth};

	float yDiff = point2.y - point1.y;
	// if points 1 and 2 are level this loop doesn't need accessing
	if (yDiff != 0.0f) {
		float invSlope1 = (point2.x - point1.x) / yDiff;
		float invSlope2 = (levelPoint.x - point1.x) / yDiff;
		float depthSlope1 = (point2.depth - point1.depth) / yDiff;
		float depthSlope2 = (levelPoint.depth - point1.depth) / yDiff;

		int yStart = (int)std::ceil(point1.y);
		int yEnd = (int)std::floor(point2.y);

		// clamp y values to window
		yStart = std::max(0, yStart);
		yEnd = std::min((int)window.height - 1, yEnd);

		float yOffset = (float)yStart - point1.y;
		float xStart = point1.x + (invSlope1 * yOffset);
		float xEnd = point1.x + (invSlope2 * yOffset);
		float dStart = point1.depth + (depthSlope1 * yOffset);
		float dEnd = point1.depth + (depthSlope2 * yOffset);

		// fill top to bottom
		// if both yStart and yEnd were out of range then yEnd < yStart so loop won't start
		for (int y = yStart; y <= yEnd; y++) {
			// temp variables so swap doesn't mess stuff up
			float xS = xStart;
			float xE = xEnd;
			float dS = dStart;
			float dE = dEnd;

			if (xS > xE) {
				std::swap(xS, xE);
				std::swap(dS, dE);
			}

			float xRange = xE - xS;
			float depthRange = dE - dS;

			int xPixelStart = (int)std::ceil(xS);
			int xPixelEnd = (int)std::floor(xE);

			// clamp
			xPixelStart = std::max(0, xPixelStart);
			xPixelEnd = std::min((int)window.width - 1, xPixelEnd);

			for (int x = xPixelStart; x <= xPixelEnd; x++) {
				float xProportion = (xRange == 0) ? 1.0f : (float)(x - xS) / xRange;
				float currentDepth = dS + (xProportion * depthRange);

				// use >= to handle shared edges cleanly
				if (currentDepth >= depthBuffer[x][y]) {
					depthBuffer[x][y] = currentDepth;
					window.setPixelColour(x, y, colourToPacked(colour));
				}
			}

			// accumlate
			xStart += invSlope1;
			xEnd += invSlope2;
			dStart += depthSlope1;
			dEnd += depthSlope2;
		}
	}

	// the same from point2 y down to point 3 y
	yDiff = point3.y - point2.y;
	// if points 2 and 3 are level then this loop doesn't need to be accessed
	if (yDiff != 0.0f) {
		float invSlope1 = (point3.x - point2.x) / yDiff;
		float invSlope2 = (point3.x - levelPoint.x) / yDiff;
		float depthSlope1 = (point3.depth - point2.depth) / yDiff;
		float depthSlope2 = (point3.depth - levelPoint.depth) / yDiff;

		int yStart = (int)std::ceil(point2.y);
		int yEnd = (int)std::floor(point3.y);

		// clamp y values to window
		yStart = std::max(0, yStart);
		yEnd = std::min((int)window.height - 1, yEnd);

		float yOffset = (float)yStart - point2.y;
		float xStart = point2.x + (invSlope1 * yOffset);
		float xEnd = levelPoint.x + (invSlope2 * yOffset);
		float dStart = point2.depth + (depthSlope1 * yOffset);
		float dEnd = levelPoint.depth + (depthSlope2 * yOffset);

		// fill top to bottom
		for (int y = yStart; y <= yEnd; y++) {
			// temp variables so swap doesn't mess stuff up
			float xS = xStart;
			float xE = xEnd;
			float dS = dStart;
			float dE = dEnd;

			if (xS > xE) {
				std::swap(xS, xE);
				std::swap(dS, dE);
			}

			float xRange = xE - xS;
			float depthRange = dE - dS;

			int xPixelStart = (int)std::ceil(xS);
			int xPixelEnd = (int)std::floor(xE);

			// clamp
			xPixelStart = std::max(0, xPixelStart);
			xPixelEnd = std::min((int)window.width - 1, xPixelEnd);

			for (int x = xPixelStart; x <= xPixelEnd; x++) {
				float xProportion = (xRange == 0) ? 1.0f : (float)(x - xS) / xRange;
				float currentDepth = dS + (xProportion * depthRange);

				// use >= to handle shared edges cleanly
				if (currentDepth >= depthBuffer[x][y]) {
					depthBuffer[x][y] = currentDepth;
					window.setPixelColour(x, y, colourToPacked(colour));
				}
			}

			// accumlate
			xStart += invSlope1;
			xEnd += invSlope2;
			dStart += depthSlope1;
			dEnd += depthSlope2;
		}
	}
}
