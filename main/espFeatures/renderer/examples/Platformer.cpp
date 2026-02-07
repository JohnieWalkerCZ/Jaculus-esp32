#include "examples/Platformer.hpp"
#include "Renderer.hpp"
#include "Shapes/Collection.hpp"
#include "Shapes/Rectangle.hpp"
#include "Utils.hpp"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include <cstdint>
#include <vector>

void init_input_gpio(int gpio_num) {
    gpio_config_t io_conf = {.pin_bit_mask = (1ULL << gpio_num),
                             .mode = GPIO_MODE_INPUT,
                             .pull_up_en = GPIO_PULLUP_ENABLE,
                             .pull_down_en = GPIO_PULLDOWN_DISABLE,
                             .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&io_conf);
}

void runPlatformer() {
#define INPUT_LEFT_GPIO GPIO_NUM_19
#define INPUT_CENTER_GPIO GPIO_NUM_20
#define INPUT_RIGHT_GPIO GPIO_NUM_21
#define TAG "PLATFORMER"

    init_input_gpio(INPUT_LEFT_GPIO);
    init_input_gpio(INPUT_CENTER_GPIO);
    init_input_gpio(INPUT_RIGHT_GPIO);

    ESP_LOGI(TAG, "Starting Platformer...");

    // --- Game Setup ---
    const int width = 64;
    const int height = 64;
    Renderer renderer(width, height, Color(40, 40, 80, 1.0f));
    DrawOptions options = {width, height, false};

    HUB75Display display(width, height);
    if (!display.isInitialized()) {
        ESP_LOGE(TAG, "Display not initialized. Halting.");
        return;
    }

    Collection *mainCollection =
        new Collection(ShapeParams{0, 0, Color(0, 0, 0, 0), 0});

    // --- Player Setup ---
    Rectangle *player = new Rectangle(
        RectangleParams{10, 48, Color(255, 80, 80, 1.0f), 5, 8, true});
    player->addCollider();
    mainCollection->addShape(player);

    // --- Platform Setup ---
    std::vector<Rectangle *> platforms;

    auto createPlatform = [&](int x, int y, int w, int h, Color c) {
        Rectangle *p = new Rectangle(RectangleParams{x, y, c, w, h, true});
        p->addCollider();
        platforms.push_back(p);
        mainCollection->addShape(p);
    };

    Color platformColor(100, 180, 80, 1.0f);
    createPlatform(0, 56, 64, 8, platformColor);  // Ground
    createPlatform(20, 46, 24, 5, platformColor); // Middle platform
    createPlatform(0, 28, 18, 5, platformColor);  // Left platform
    createPlatform(46, 28, 18, 5, platformColor); // Right platform
    createPlatform(22, 14, 20, 5, platformColor); // Top-middle platform

    // --- Physics and Game State ---
    float velocityX = 0.0f;
    float velocityY = 0.0f;
    const float gravity = 0.25f;
    const float jumpForce = -3.8f;
    const float moveSpeed = 1.8f;
    bool canJump = false;

    Pixels pixels;

    // --- Game Loop ---
    while (1) {
        uint64_t frameStartTime = esp_timer_get_time();

        // --- Input Handling ---
        bool left_pressed = !gpio_get_level(INPUT_LEFT_GPIO);
        bool right_pressed = !gpio_get_level(INPUT_RIGHT_GPIO);
        bool jump_pressed = !gpio_get_level(INPUT_CENTER_GPIO);

        velocityX = 0;
        if (left_pressed) {
            velocityX = -moveSpeed;
        }
        if (right_pressed) {
            velocityX = moveSpeed;
        }
        if (jump_pressed && canJump) {
            velocityY = jumpForce;
            canJump = false;
        }

        // --- Physics (Separated Axis) ---

        // 1. Horizontal Movement
        player->translate(velocityX, 0.0f);

        // 2. Vertical Movement & Collision
        velocityY += gravity;
        float oldPlayerY = player->getY();
        player->translate(0.0f, velocityY);
        canJump = false;
        for (auto *platform : platforms) {
            if (player->intersects(platform)) {

                if (velocityY > 0 &&
                    oldPlayerY + player->getHeight() <= platform->getY() + 1) {
                    player->setPosition(player->getX(),
                                        platform->getY() - player->getHeight());
                    velocityY = 0;
                    canJump = true;
                }
            }
        }

        // --- Screen Boundaries & Respawn ---
        if (player->getX() < 0) {
            player->setPosition(0, player->getY());
        }
        if (player->getX() + player->getWidth() > width) {
            player->setPosition(width - player->getWidth(), player->getY());
        }
        if (player->getY() > height) {
            player->setPosition(10, 48);
            velocityY = 0;
        }

        // --- Rendering ---
        pixels.clear();
        renderer.render(pixels, std::vector<Collection *>{mainCollection},
                        options);
        display.setBuffer(pixels);

        // --- Frame Rate Control ---
        const uint64_t TARGET_FRAME_TIME_US = 1000000 / 30;
        uint64_t frameEndTime = esp_timer_get_time();
        uint64_t frameTime = frameEndTime - frameStartTime;
        if (frameTime < TARGET_FRAME_TIME_US) {
            uint64_t remainingTime = TARGET_FRAME_TIME_US - frameTime;
            vTaskDelay(pdMS_TO_TICKS(remainingTime / 1000));
        }
    }

    delete mainCollection;
}
