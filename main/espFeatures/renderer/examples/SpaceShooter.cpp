#include "examples/SpaceShooter.hpp"
#include "Font.hpp"
#include "RegularPolygon.hpp"
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
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <string>
#include <vector>

// Copied from main.cpp, required by the examples
void init_input_gpio(int gpio_num) {
    gpio_config_t io_conf = {.pin_bit_mask = (1ULL << gpio_num),
                             .mode = GPIO_MODE_INPUT,
                             .pull_up_en = GPIO_PULLUP_ENABLE,
                             .pull_down_en = GPIO_PULLDOWN_DISABLE,
                             .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&io_conf);
}

void runSpaceShooter() {
#define INPUT_LEFT_GPIO GPIO_NUM_19
#define INPUT_CENTER_GPIO GPIO_NUM_20
#define INPUT_RIGHT_GPIO GPIO_NUM_21
#define TAG "SPACESHOOTER"

    // --- Setup ---
    init_input_gpio(INPUT_LEFT_GPIO);
    init_input_gpio(INPUT_CENTER_GPIO);
    init_input_gpio(INPUT_RIGHT_GPIO);

    const int width = 64;
    const int height = 64;
    Renderer renderer(width, height, Color(0, 0, 15, 1.0f)); // Deep space blue
    DrawOptions options = {width, height, false};

    HUB75Display display(width, height);
    if (!display.isInitialized()) {
        ESP_LOGE(TAG, "Display not initialized. Halting.");
        return;
    }
    Font font = defaultFont;

    // --- Game Objects & State ---
    int score = 0;
    int shoot_cooldown = 0;
    int enemy_spawn_timer = 0;
    bool game_over = false;

    // --- Player ---
    Collection *player_collection =
        new Collection({0, 0, Color(0, 0, 0, 0), 0});
    RegularPolygon *player = new RegularPolygon(RegularPolygonRadiusParams{
        width / 2.0f, height - 8.0f, Color(0, 255, 100, 1.0f), 3, 4, true});
    player->addCollider();
    player_collection->addShape(player);

    // --- Object Pools ---
    const int MAX_BULLETS = 15;
    const int MAX_ENEMIES = 20;
    const float OFF_SCREEN_POS = -20.0f;

    Collection *bullet_collection =
        new Collection({0, 0, Color(0, 0, 0, 0), 0});
    std::vector<Rectangle *> bullet_pool;
    std::vector<bool> bullet_active;

    Collection *enemy_collection = new Collection({0, 0, Color(0, 0, 0, 0), 0});
    std::vector<Rectangle *> enemy_pool;
    std::vector<bool> enemy_active;

    for (int i = 0; i < MAX_BULLETS; ++i) {
        Rectangle *b = new Rectangle(
            RectangleParams{OFF_SCREEN_POS, OFF_SCREEN_POS,
                            Color(255, 255, 0, 1.0f), 2, 4, true});
        b->addCollider();
        bullet_pool.push_back(b);
        bullet_active.push_back(false);
        bullet_collection->addShape(b);
    }

    for (int i = 0; i < MAX_ENEMIES; ++i) {
        Rectangle *e = new Rectangle(
            RectangleParams{OFF_SCREEN_POS, OFF_SCREEN_POS,
                            Color(255, 50, 50, 1.0f), 5, 5, true});
        e->addCollider();
        enemy_pool.push_back(e);
        enemy_active.push_back(false);
        enemy_collection->addShape(e);
    }

    std::vector<Collection *> all_collections = {
        player_collection, bullet_collection, enemy_collection};

    auto resetGame = [&]() {
        player->setPosition(width / 2.0f, height - 8.0f);
        for (int i = 0; i < MAX_BULLETS; ++i) {
            bullet_active[i] = false;
            bullet_pool[i]->setPosition(OFF_SCREEN_POS, OFF_SCREEN_POS);
        }
        for (int i = 0; i < MAX_ENEMIES; ++i) {
            enemy_active[i] = false;
            enemy_pool[i]->setPosition(OFF_SCREEN_POS, OFF_SCREEN_POS);
        }
        score = 0;
        shoot_cooldown = 0;
        enemy_spawn_timer = 0;
        game_over = false;
        ESP_LOGI(TAG, "Game Start!");
    };

    resetGame();

    Pixels pixels;
    const uint64_t TARGET_FRAME_TIME_US = 1000000 / 30;

    // --- Game Loop ---
    while (1) {
        uint64_t frameStartTime = esp_timer_get_time();

        bool left_pressed = !gpio_get_level(INPUT_LEFT_GPIO);
        bool right_pressed = !gpio_get_level(INPUT_RIGHT_GPIO);
        bool action_pressed = !gpio_get_level(INPUT_CENTER_GPIO);

        if (game_over) {
            if (action_pressed) {
                resetGame();
            }
        } else {
            // --- Player Movement ---
            float player_speed = 1.5f;
            if (left_pressed && player->getX() > player->getRadius()) {
                player->translate(-player_speed, 0.0f);
            }
            if (right_pressed && player->getX() < width - player->getRadius()) {
                player->translate(player_speed, 0.0f);
            }

            // --- Shooting ---
            if (shoot_cooldown > 0)
                shoot_cooldown--;
            if (action_pressed && shoot_cooldown == 0) {
                for (int i = 0; i < MAX_BULLETS; ++i) {
                    if (!bullet_active[i]) {
                        bullet_active[i] = true;
                        bullet_pool[i]->setPosition(player->getX() - 1,
                                                    player->getY() -
                                                        player->getRadius());
                        shoot_cooldown = 10; // 10 frames cooldown
                        break;
                    }
                }
            }

            // --- Enemy Spawning ---
            enemy_spawn_timer--;
            if (enemy_spawn_timer <= 0) {
                for (int i = 0; i < MAX_ENEMIES; ++i) {
                    if (!enemy_active[i]) {
                        enemy_active[i] = true;
                        int rand_x = 5 + (rand() % (width - 10));
                        enemy_pool[i]->setPosition(rand_x, -5);
                        enemy_spawn_timer = 20 + (rand() % 20);
                        break;
                    }
                }
            }

            // --- Updates & Collisions ---
            // Update active bullets
            for (int i = 0; i < MAX_BULLETS; ++i) {
                if (bullet_active[i]) {
                    bullet_pool[i]->translate(0.0f, -4.0f);
                    if (bullet_pool[i]->getY() < 0) {
                        bullet_active[i] = false;
                        bullet_pool[i]->setPosition(OFF_SCREEN_POS,
                                                    OFF_SCREEN_POS);
                    }
                }
            }

            // Update active enemies and check player collision
            float enemy_speed = 1.0f + (score / 150.0f);
            for (int i = 0; i < MAX_ENEMIES; ++i) {
                if (enemy_active[i]) {
                    enemy_pool[i]->translate(0.0f, enemy_speed);
                    if (enemy_pool[i]->getY() > height) {
                        enemy_active[i] = false;
                        enemy_pool[i]->setPosition(OFF_SCREEN_POS,
                                                   OFF_SCREEN_POS);
                    }
                    if (player->intersects(enemy_pool[i])) {
                        game_over = true;
                        ESP_LOGI(TAG, "Game Over! Final Score: %d", score);
                    }
                }
            }

            // Bullet-Enemy collisions
            for (int i = 0; i < MAX_BULLETS; ++i) {
                if (bullet_active[i]) {
                    for (int j = 0; j < MAX_ENEMIES; ++j) {
                        if (enemy_active[j] &&
                            bullet_pool[i]->intersects(enemy_pool[j])) {
                            bullet_active[i] = false;
                            bullet_pool[i]->setPosition(OFF_SCREEN_POS,
                                                        OFF_SCREEN_POS);
                            enemy_active[j] = false;
                            enemy_pool[j]->setPosition(OFF_SCREEN_POS,
                                                       OFF_SCREEN_POS);
                            score += 10;
                            break;
                        }
                    }
                }
            }
        }

        // --- Rendering ---
        pixels.clear();
        if (game_over) {
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    pixels.push_back(Pixel(x, y, Color(100, 0, 0, 1.0f)));
                }
            }
            renderer.drawText(pixels, "GAME OVER", 8, height / 2 - 8, font,
                              Color(255, 255, 255, 1.0f));
            renderer.drawText(pixels, "SCORE: " + std::to_string(score), 7,
                              height / 2 + 8, font, Color(255, 255, 255, 1.0f));
        } else {
            renderer.render(pixels, all_collections, options);
            renderer.drawText(pixels, std::to_string(score), 0, 0, font,
                              Color(0, 255, 0, 1.0f));
        }
        display.setBuffer(pixels);

        // --- Frame Rate Control ---
        uint64_t frameEndTime = esp_timer_get_time();
        uint64_t frameTime = frameEndTime - frameStartTime;
        if (frameTime < TARGET_FRAME_TIME_US) {
            uint64_t remainingTime = TARGET_FRAME_TIME_US - frameTime;
            vTaskDelay(pdMS_TO_TICKS(remainingTime / 1000));
        }
    }

    // Cleanup
    delete player_collection;
    delete bullet_collection;
    delete enemy_collection;
}
