#include "examples/TextExample.hpp"
#include "Font/Font.hpp"
#include "Renderer.hpp"

void runTextExample() {
    int width = 64;
    int height = 64;
    Renderer renderer(width, height);
    Font font = defaultFont;
    Pixels pixels;

    HUB75Display display(width, height);
    if (!display.isInitialized()) {
        return;
    }

    display.setBrightness(100);

    while (1) {
        pixels.clear();
        renderer.drawText(pixels, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", 0, 0, font,
                          {255, 0, 0, 1.0f}, true);

        renderer.drawText(pixels, "abcdefghijklmnopqrstuvwxyz", 0, 24, font,
                          {0, 255, 0, 1.0f}, true);
        renderer.drawText(pixels, "0123456789", 0, 48, font, {0, 0, 255, 1.0f},
                          true);

        display.setBuffer(pixels);
        vTaskDelay(pdMS_TO_TICKS(10000));
        pixels.clear();
        renderer.drawText(pixels, "!\"#$%&'()*+,-./:;<=>?@[\\]^_`|~ ", 0, 0,
                          font, {255, 255, 0, 1.0f}, true);
        display.setBuffer(pixels);
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
