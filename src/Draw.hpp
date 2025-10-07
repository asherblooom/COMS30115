#ifndef DRAW_HPP
#define DRAW_HPP

#include <CanvasPoint.h>
#include <CanvasTriangle.h>
#include <Colour.h>
#include <DrawingWindow.h>
#include <TextureMap.h>

#define WIDTH 1920
#define HEIGHT 1060

void draw(DrawingWindow &window, float x, float y, Colour &colour);

void drawLine(DrawingWindow &window, CanvasPoint from, CanvasPoint to, Colour colour = {255, 255, 255});

void drawBaryTriangle(DrawingWindow &window, CanvasTriangle triangle);

void drawStokedTriangle(DrawingWindow &window, CanvasTriangle triangle, Colour colour = {255, 255, 255});

void drawFilledTriangle(DrawingWindow &window, CanvasTriangle triangle, Colour colour = {255, 255, 255});

void drawTexturedTriangle(DrawingWindow &window, CanvasTriangle triangle, TextureMap texture);

#endif
