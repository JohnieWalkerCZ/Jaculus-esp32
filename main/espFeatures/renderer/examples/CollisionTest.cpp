#include "examples/CollisionTest.hpp"
#include "RegularPolygon.hpp"
#include "Renderer.hpp"
#include "Shapes/Collection.hpp"
#include "Shapes/Rectangle.hpp"
#include "Utils.hpp"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include <cstdio>
#include <stdio.h>

void runCollisionTest() {
    const int width = 64;
    const int height = 64;
    Renderer renderer(width, height, Color(0, 0, 0, 1.0f));
    Collection *mainCollection =
        new Collection(ShapeParams{32, 32, Color(0, 0, 0, 1.0f), 0});

    RegularPolygon *triangle = new RegularPolygon(
        RegularPolygonRadiusParams{32, 60, Color(0, 255, 0, 1.0f), 3, 4, true});
    Rectangle *enemy = new Rectangle(
        RectangleParams{20, 0, Color(255, 0, 0, 1.0f), 10, 10, true});
    triangle->addCollider();
    enemy->addCollider();

    mainCollection->addShape(triangle);
    mainCollection->addShape(enemy);

    DrawOptions options = {width, height, true}; // Anti-aliased
    HUB75Display display(width, height);
    if (!display.isInitialized()) {
        printf("Display not initialized. Skipping display output.\n");
        return;
    }

    printf("Triangle Collider position: (%.2f, %.2f)\n",
           triangle->getCollider()->x, triangle->getCollider()->y);
    Pixels pixels;
    while (true) {
        pixels.clear();

        enemy->translate(0, 1);

        if (triangle->intersects(enemy)) {
            triangle->setColor(Color(255, 0, 0, 1.0f)); // Red on collision
            printf("Collision detected at enemy position (%.2f, %.2f)\n",
                   enemy->getCollider()->x, enemy->getCollider()->y);
        } else {
            triangle->setColor(Color(0, 255, 0, 1.0f)); // Green otherwise
        }

        renderer.render(pixels, std::vector<Collection *>{mainCollection},
                        options);
        display.setBuffer(pixels);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
