#include "TextureTest.hpp"
#include "Collection.hpp"
#include "Rectangle.hpp"
#include "Renderer.hpp"
#include "Shape.hpp"
#include "Texture.hpp"
#include "Utils.hpp"
#include "esp_log.h"

static const char *TAG = "TextureTest";

void runTextureTest() {
    ESP_LOGI(TAG, "Starting Texture Test");

    if (!Texture::initFS()) {
        ESP_LOGE(TAG, "Failed to initialize filesystem for Texture Test");
        return;
    }

    Texture texture;
    if (!Texture::fromBMP("brick-contrast.bmp", texture, "repeat")) {
        ESP_LOGE(TAG, "Failed to load texture from BMP");
        return;
    }
    const int width = 64;
    const int height = 64;
    Renderer renderer(width, height);

    HUB75Display display(width, height);
    if (!display.isInitialized()) {
        ESP_LOGE(TAG, "Display not initialized");
        return;
    }
    display.setBrightness(100);

    Collection *scene = new Collection(ShapeParams{29, 29, Colors::BLACK, 0});

    Rectangle *texturedRect =
        new Rectangle(RectangleParams{-13, -13, Colors::WHITE, 91, 91, true});
    texturedRect->setTexture(&texture);
    texturedRect->setFixTexture(true);
    texturedRect->setTextureScale(3.0f, 3.0f);
    texturedRect->setPivot(32, 32);

    scene->addShape(texturedRect);
    DrawOptions options = {width, height, true};
    Pixels pixels;
    while (1) {
        pixels.clear();
        texturedRect->rotate(10.0f);
        renderer.render(pixels, {scene}, options);
        display.setBuffer(pixels);
        vTaskDelay(pdMS_TO_TICKS(1000 / 60));
    }
}
