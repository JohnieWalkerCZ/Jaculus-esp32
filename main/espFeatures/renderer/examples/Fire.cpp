#include "examples/Fire.hpp"
#include "Utils.hpp"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdio.h>
#include <vector>

void runFire() {
    const int width = 64;
    const int height = 64;

    const int COOLING =
        20; // Value from 0-100. Higher value means faster cooling.

    const int SPARKING =
        255; // Value from 0-255. Higher value means more sparks.

    const int FPS = 30;

    HUB75Display display(width, height);

    if (!display.isInitialized()) {

        printf("Display not initialized. Skipping display output.\n");

        return;
    }

    // 2D buffer for fire intensity values, allocated on the heap
    std::vector<uint8_t> fire(width * height, 0);

    // Fire color palette (Black -> Red -> Yellow -> White)
    std::vector<Color> palette(256);
    for (int i = 0; i < 256; i++) {
        if (i < 32) { // Black to Red
            palette[i] = Color(i * 8, 0, 0);
        } else if (i < 96) { // Red to Yellow
            palette[i] = Color(255, (i - 32) * 4, 0);
        } else if (i < 160) { // Yellow to White
            palette[i] = Color(255, 255, (i - 96) * 4);
        } else { // White
            palette[i] = Color(255, 255, 255);
        }
    }

    Pixels pixels;
    while (true) {
        // 1. Seed the fire source at the bottom
        for (int x = 0; x < width; x++) {
            if (rand() % 255 < SPARKING) {
                fire[(height - 1) * width + x] = rand() % 256;
            }
        }

        // 2. Propagate fire upwards and cool it down
        for (int y = 0; y < height - 1; y++) {
            for (int x = 0; x < width; x++) {
                // Average surrounding pixels from below
                uint8_t new_val =
                    (fire[(y + 1) * width + ((x - 1 + width) % width)] +
                     fire[(y + 1) * width + x] +
                     fire[(y + 1) * width + ((x + 1) % width)] +
                     fire[((y + 2) % height) * width + x]) /
                    4;

                // Cooling
                int random_cool = rand() % (COOLING / 4 + 1);

                fire[y * width + x] =
                    (new_val > random_cool) ? new_val - random_cool : 0;
            }
        }

        // Blur the bottom row a bit to make it less "column-y"
        for (int x = 0; x < width; x++) {
            int y = height - 1;
            fire[y * width + x] = (fire[y * width + x] +
                                   fire[y * width + ((x - 1 + width) % width)] +
                                   fire[y * width + ((x + 1) % width)]) /
                                  3;
        }

        // 3. Render fire buffer to the display buffer
        pixels.clear();
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                uint8_t temp = fire[y * width + x];

                if (temp > 0) {
                    pixels.push_back(Pixel(x, y, palette[temp]));
                }
            }
        }

        display.setBuffer(pixels);
        vTaskDelay(pdMS_TO_TICKS(1000 / FPS));
    }
}
