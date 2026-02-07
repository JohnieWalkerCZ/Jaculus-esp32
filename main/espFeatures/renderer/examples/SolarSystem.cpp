#include "examples/SolarSystem.hpp"
#include "Polygon.hpp"
#include "Renderer.hpp"
#include "Shapes/Circle.hpp"
#include "Shapes/Collection.hpp"
#include "Utils.hpp"
#include "esp_timer.h"
#include "freertos/idf_additions.h"
#include "freertos/task.h"
#include "portmacro.h"
#include <cstdint>
#include <cstdio>
#include <stdio.h>
#include <vector>

void runSolarSystem() {
    const int width = 64;
    const int height = 64;
    Renderer renderer(width, height, Color(0, 0, 0, 1.0f));

    std::vector<Collection *> collections;

    Collection *sunCollection =
        new Collection(ShapeParams{32, 32, Color(0, 0, 0, 1.0f), 0});
    sunCollection->setPivot(32.0f, 32.0f);
    collections.push_back(sunCollection);

    Circle *sun =
        new Circle(CircleParams{32, 32, Color(255, 204, 0, 1.0f), 8, true});
    sunCollection->addShape(sun);

    std::vector<std::pair<int, int>> polygonVertices = {
        {60, 60}, {63, 61}, {61, 63}};
    Polygon *polygon = new Polygon(
        PolygonParams{32, 32, Color(255, 150, 0, 1.0f), polygonVertices, true});
    sunCollection->addShape(polygon);

    Collection *earthCollection =
        new Collection(ShapeParams{32, 32, Color(0, 0, 0, 1.0f), 1});
    earthCollection->setPivot(32.0f, 32.0f);

    Circle *earth = new Circle(
        CircleParams{32 + 20, 32, Color(0, 100, 255, 1.0f), 4, true});
    earthCollection->addShape(earth);

    Circle *alien = new Circle(
        CircleParams{32 - 20, 32, Color(0, 255, 100, 1.0f), 4, true});
    earthCollection->addShape(alien);

    Collection *moonCollection =
        new Collection(ShapeParams{32 + 20, 32, Color(0, 0, 0, 1.0f), 1});
    Circle *moon = new Circle(
        CircleParams{32 + 20 + 8, 32, Color(200, 200, 200, 1.0f), 2, true});
    moonCollection->addShape(moon);

    earthCollection->addShape(moonCollection);
    sunCollection->addShape(earthCollection);

    Circle *earthOrbit =
        new Circle(CircleParams{32, 32, Color(100, 100, 100, 0.3f), 20, false});
    Circle *moonOrbit = new Circle(
        CircleParams{32 + 20, 32, Color(100, 100, 100, 0.3f), 8, false});
    sunCollection->addShape(earthOrbit);
    earthCollection->addShape(moonOrbit);

    DrawOptions options = {width, height, true}; // Anti-aliased

    HUB75Display display(width, height);
    if (!display.isInitialized()) {
        printf("Display not initialized. Skipping display output.\n");
        return;
    }

    display.clear();
    int counter = 0;
    uint64_t sumTime = 0;
    uint64_t renderTime = 0;

    const float TARGET_FPS = 200.0f;
    const uint64_t TARGET_FRAME_TIME_US = 1000000 / TARGET_FPS;
    Pixels pixels;
    while (true) {
        pixels.clear();
        uint64_t frameStartTime = esp_timer_get_time();

        earthCollection->rotate(1.5f);
        moonCollection->rotate(3.0f);

        renderer.render(pixels, collections, options);
        display.setBuffer(pixels);

        uint64_t frameEndTime = esp_timer_get_time();
        uint64_t frameTime = frameEndTime - frameStartTime;

        vTaskDelay(1);
        if (frameTime < TARGET_FRAME_TIME_US) {
            uint64_t remainingTime = TARGET_FRAME_TIME_US - frameTime;
            TickType_t delayTicks =
                ((remainingTime / 1000) / portTICK_PERIOD_MS);
            if (delayTicks > 0) {
                vTaskDelay(delayTicks);
            }
        }

        uint64_t adjustedFrameEndTime = esp_timer_get_time();
        uint64_t adjustedFrameTime = adjustedFrameEndTime - frameStartTime;
        sumTime += adjustedFrameTime;
        renderTime += frameTime;
        counter++;
        if (counter % 30 == 0) {
            float avgFrameTimeMs = (sumTime / counter) / 1000.0f;
            float currentFps = 1000000.0f / (avgFrameTimeMs * 1000.0f);

            printf("%.2f ms/frame (%.1f FPS) over %d frames\n", avgFrameTimeMs,
                   currentFps, counter);
            printf("  Render time: %.2f ms/frame\n",
                   (renderTime / counter) / 1000.0f);

            counter = 0;
            sumTime = 0;
            renderTime = 0;
        }
    }
    // Cleanup
    for (auto collection : collections) {
        delete collection;
    }
}
