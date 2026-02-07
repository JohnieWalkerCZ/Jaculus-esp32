#include "examples/TestShapes.hpp"
#include "LineSegment.hpp"
#include "Point.hpp"
#include "Polygon.hpp"
#include "RegularPolygon.hpp"
#include "Renderer.hpp"
#include "Shapes/Circle.hpp"
#include "Shapes/Collection.hpp"
#include "Shapes/Rectangle.hpp"
#include "Utils.hpp"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include <cstdio>
#include <stdio.h>

void runTestShapes() {
    const int width = 64;
    const int height = 64;

    Renderer renderer(width, height, Color(0, 0, 0, 1.0f));

    Collection *mainCollection =
        new Collection(ShapeParams{32, 32, Color(0, 0, 0, 1.0f), 0});

    Rectangle *rect = new Rectangle(
        RectangleParams{6, 6, Color(255, 0, 0, 1.0f), 6, 6, true});

    Circle *circle =
        new Circle(CircleParams{18, 6, Color(0, 0, 255, 1.0f), 5, true});

    RegularPolygon *hexagon = new RegularPolygon(RegularPolygonRadiusParams{
        48, 48, Color(0, 255, 0, 1.0f), 6, 10, true});
    RegularPolygon *pentagon = new RegularPolygon(RegularPolygonRadiusParams{
        48, 16, Color(255, 255, 0, 1.0f), 5, 8, true});

    Polygon *polygon = new Polygon(PolygonParams{
        16, 48, Color(255, 0, 255, 1.0f),
        std::vector<std::pair<int, int>>{{0, 0}, {10, 5}, {5, 15}, {0, 10}},
        true});

    LineSegment *line = new LineSegment(
        LineSegmentParams{0, 20, Color(255, 255, 255, 1.0f), 63, 35, 0});

    LineSegment *line2 = new LineSegment(
        LineSegmentParams{32, 0, Color(0, 100, 50, .5f), 20, 40, 0});

    Rectangle *lineRect = new Rectangle(
        RectangleParams{25, 20, Color(155, 0, 0, 0.9f), 8, 5, true});

    Point *point =
        new Point(ShapeParams{32, 42, Color(155, 255, 255, 1.0f), 0});

    mainCollection->addShape(rect);
    mainCollection->addShape(circle);
    mainCollection->addShape(hexagon);
    mainCollection->addShape(pentagon);
    mainCollection->addShape(polygon);
    mainCollection->addShape(line);
    mainCollection->addShape(line2);
    mainCollection->addShape(lineRect);
    mainCollection->addShape(point);

    DrawOptions options = {width, height, true}; // Anti-aliased
    HUB75Display display(width, height);
    if (!display.isInitialized()) {
        printf("Display not initialized. Skipping display output.\n");
        return;
    }

    Pixels pixels;
    renderer.render(pixels, std::vector<Collection *>{mainCollection}, options);
    while (true) {
        display.setBuffer(pixels);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Cleanup
    delete mainCollection;
}
