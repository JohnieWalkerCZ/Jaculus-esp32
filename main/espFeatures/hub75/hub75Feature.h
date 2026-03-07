#pragma once

#include "../renderer/rendererFeature.h"
#include "ESP32-HUB75-MatrixPanel-I2S-DMA.h"
#include "driver/gpio.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "jac/device/logger.h"
#include <cstdint>
#include <cstring>
#include <format>
#include <jac/machine/class.h>
#include <jac/machine/functionFactory.h>
#include <jac/machine/machine.h>
#include <jac/machine/values.h>
#include <vector>
#include "Utils.hpp"

static MatrixPanel_I2S_DMA *s_display = nullptr;

template <class Next> class Hub75Feature;

class Hub75Holder {
  public:
    static Hub75Holder *active_instance;

  private:
    MatrixPanel_I2S_DMA *display;

    Color *m_previousBuffer;
    Color *m_current_frame_buffer;
    uint8_t *m_pixel_is_set;

    int m_width;
    int m_height;
    int m_chain_length;
    size_t m_buffer_count;
    bool m_initialized;

    static const char *TAG;

    HUB75_I2S_CFG::i2s_pins getDefaultPins() {
        return {.r1 = 4,
                .g1 = 5,
                .b1 = 6,
                .r2 = 7,
                .g2 = 15,
                .b2 = 16,
                .a = 18,
                .b = 8,
                .c = 10,
                .d = 42,
                .e = 17,
                .lat = 47,
                .oe = 2,
                .clk = 48};
    }

    bool isValidCoordinate(int x, int y) const {
        return x >= 0 && x < m_width && y >= 0 && y < m_height;
    }

  public:
    Hub75Holder(int panelWidth, int panelHeight, int chainLength)
        : display(nullptr), m_previousBuffer(nullptr),
          m_current_frame_buffer(nullptr), m_pixel_is_set(nullptr),
          m_initialized(false) {

        active_instance = this;

        m_width = panelWidth * chainLength;
        m_height = panelHeight;
        m_chain_length = chainLength;
        m_buffer_count = m_width * m_height;
    }

    ~Hub75Holder() {
        if (m_current_frame_buffer)
            heap_caps_free(m_current_frame_buffer);
        if (m_previousBuffer)
            heap_caps_free(m_previousBuffer);
        if (m_pixel_is_set)
            heap_caps_free(m_pixel_is_set);

        if (active_instance == this) {
            active_instance = nullptr;
        }
    }

    void start() {
        if (s_display != nullptr) {
            jac::Logger::debug("Hub75Holder: Soft-Reload detected. Hardware "
                               "requires clean state.");
            jac::Logger::debug("Hub75Holder: Rebooting device...");

            vTaskDelay(pdMS_TO_TICKS(100));
            esp_restart();

            while (true)
                vTaskDelay(100);
        }

        jac::Logger::debug("Hub75Holder: Starting Fresh Initialization...");

        if (m_previousBuffer) {
            heap_caps_free(m_previousBuffer);
            m_previousBuffer = nullptr;
        }
        if (m_current_frame_buffer) {
            heap_caps_free(m_current_frame_buffer);
            m_current_frame_buffer = nullptr;
        }
        if (m_pixel_is_set) {
            heap_caps_free(m_pixel_is_set);
            m_pixel_is_set = nullptr;
        }

        m_initialized = false;

        HUB75_I2S_CFG mxconfig(m_width / m_chain_length, m_height,
                               m_chain_length, getDefaultPins());
        mxconfig.double_buff = false;

        s_display = new MatrixPanel_I2S_DMA(mxconfig);

        if (!s_display || !s_display->begin()) {
            jac::Logger::error(
                "Hub75Holder: Hardware Init Failed! Rebooting...");
            vTaskDelay(pdMS_TO_TICKS(200));
            esp_restart();
            return;
        }

        this->display = s_display;

        jac::Logger::debug("Hub75Holder: Allocating PSRAM...");
        uint32_t caps = MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT;
        m_previousBuffer =
            (Color *)heap_caps_malloc(m_buffer_count * sizeof(Color), caps);
        m_current_frame_buffer =
            (Color *)heap_caps_malloc(m_buffer_count * sizeof(Color), caps);
        m_pixel_is_set =
            (uint8_t *)heap_caps_calloc(m_buffer_count, sizeof(uint8_t), caps);

        if (!m_previousBuffer || !m_current_frame_buffer || !m_pixel_is_set) {
            jac::Logger::error("Hub75Holder: Out of Memory (PSRAM)");
            return;
        }

        for (size_t i = 0; i < m_buffer_count; i++) {
            m_previousBuffer[i] = Colors::BLACK;
            m_current_frame_buffer[i] = Colors::BLACK;
        }

        display->setBrightness8(90);
        display->clearScreen();

        m_initialized = true;
        jac::Logger::debug("Hub75Holder: Initialization COMPLETE.");
    }

    void setPixel(int x, int y, const Color &color) {
        if (!m_initialized || !isValidCoordinate(x, y))
            return;
        display->drawPixelRGB888(x, y, (uint8_t)(color.r * color.a),
                                 (uint8_t)(color.g * color.a),
                                 (uint8_t)(color.b * color.a));
    }

    void setBuffer(const Pixels &pixels, bool clearPrevious) {
        if (!m_initialized || !m_current_frame_buffer)
            return;

        memset(m_pixel_is_set, 0, m_buffer_count * sizeof(uint8_t));

        for (const auto &pixel : pixels) {
            if (isValidCoordinate(pixel.x, pixel.y)) {
                int index = pixel.y * m_width + pixel.x;
                m_current_frame_buffer[index] = pixel.color;
                m_pixel_is_set[index] = 1;
            }
        }

        for (int i = 0; i < m_buffer_count; ++i) {
            Color new_color;
            if (m_pixel_is_set[i]) {
                new_color = m_current_frame_buffer[i];
            } else {
                new_color = clearPrevious ? Colors::BLACK : m_previousBuffer[i];
            }

            if (m_previousBuffer[i] != new_color) {
                int y = i / m_width;
                int x = i % m_width;
                setPixel(x, y, new_color);
                m_previousBuffer[i] = new_color;
            }
        }
    }

    void setBufferFromRaw(const uint8_t *rawData, size_t size,
                          bool clearPrevious) {
        if (!m_initialized || !m_current_frame_buffer)
            return;

        memset(m_pixel_is_set, 0, m_buffer_count * sizeof(uint8_t));
        for (size_t i = 0; i < size; i += 8) {
            int16_t x = (int16_t)(rawData[i] | (rawData[i + 1] << 8));
            int16_t y = (int16_t)(rawData[i + 2] | (rawData[i + 3] << 8));

            if (isValidCoordinate(x, y)) {
                uint8_t r = rawData[i + 4];
                uint8_t g = rawData[i + 5];
                uint8_t b = rawData[i + 6];
                float a = rawData[i + 7] / 255.0f;

                int index = y * m_width + x;
                m_current_frame_buffer[index] = Color(r, g, b, a);
                m_pixel_is_set[index] = 1;
            }
        }

        for (int i = 0; i < m_buffer_count; ++i) {
            Color new_color;
            if (m_pixel_is_set[i]) {
                new_color = m_current_frame_buffer[i];
            } else {
                new_color = clearPrevious ? Colors::BLACK : m_previousBuffer[i];
            }

            if (m_previousBuffer[i] != new_color) {
                int y = i / m_width;
                int x = i % m_width;
                setPixel(x, y, new_color);
                m_previousBuffer[i] = new_color;
            }
        }
    }

    void clear() {
        if (m_initialized) {
            display->clearScreen();
            for (size_t i = 0; i < m_buffer_count; i++) {
                m_previousBuffer[i] = Colors::BLACK;
            }
        }
    }

    void setBrightness(uint8_t brightness) {
        if (m_initialized)
            display->setBrightness8(brightness);
    }

    bool isInitialized() const { return m_initialized; }
};

inline const char *Hub75Holder::TAG = "Hub75Holder";
template <> struct jac::ConvTraits<Pixels> {
    static Pixels from(ContextRef ctx, ValueWeak val) {
        auto array = val.to<jac::ArrayWeak>();
        Pixels pixels;
        uint32_t len = array.length();
        pixels.reserve(len);
        for (uint32_t i = 0; i < len; ++i) {
            auto pixelArray = array.get(i).to<jac::ArrayWeak>();
            pixels.emplace_back(
                pixelArray.get(0).to<int>(), pixelArray.get(1).to<int>(),
                Color(pixelArray.get(2).to<uint8_t>(),
                      pixelArray.get(3).to<uint8_t>(),
                      pixelArray.get(4).to<uint8_t>(),
                      pixelArray.length() > 5 ? pixelArray.get(5).to<float>()
                                              : 1.0f));
        }
        return pixels;
    }
};

template <class Next> class Hub75Feature : public Next {
  private:
    static Hub75Holder *s_instance;

  public:
    class Hub75ProtoBuilder : public jac::ProtoBuilder::Opaque<Hub75Holder>,
                              public jac::ProtoBuilder::Properties {
      public:
        static Hub75Holder *constructOpaque(jac::ContextRef ctx,
                                            std::vector<jac::ValueWeak> args) {
            int pW = args.size() > 0 ? args[0].to<int>() : 64;
            int pH = args.size() > 1 ? args[1].to<int>() : 32;
            int cL = args.size() > 2 ? args[2].to<int>() : 1;

            s_instance = new Hub75Holder(pW, pH, cL);
            s_instance->start();
            return s_instance;
        }

        static void addProperties(jac::ContextRef ctx, jac::Object proto) {
            jac::FunctionFactory ff(ctx);
            proto.defineProperty(
                "setBufferDirect",
                ff.newFunctionThis([](jac::ContextRef ctx,
                                      jac::ValueWeak thisVal,
                                      jac::ValueWeak bufferVal, bool clear) {
                    auto *holder = getOpaque(ctx, thisVal);
                    if (!holder)
                        return;
                    auto *fb = FrameBufferProtoBuilder::unwrap(ctx, bufferVal);
                    if (!fb || !fb->data)
                        return;
                    holder->setBufferFromRaw(fb->data, fb->size, clear);
                    if (clear)
                        fb->size = 0;
                }),
                jac::PropFlags::Enumerable);

            proto.defineProperty(
                "setBuffer",
                ff.newFunctionThis([](jac::ContextRef ctx,
                                      jac::ValueWeak thisVal,
                                      jac::ValueWeak bufferVal, bool clear) {
                    auto *holder = getOpaque(ctx, thisVal);
                    if (!holder)
                        return;
                    size_t len;
                    uint8_t *buf =
                        JS_GetArrayBuffer(ctx, &len, bufferVal.getVal());
                    if (!buf || len == 0)
                        return;
                    Pixels pixels;
                    size_t count = len / 8;
                    pixels.reserve(count);
                    for (size_t i = 0; i < len; i += 8) {
                        int16_t x = buf[i] | (buf[i + 1] << 8);
                        int16_t y = buf[i + 2] | (buf[i + 3] << 8);
                        uint8_t r = buf[i + 4];
                        uint8_t g = buf[i + 5];
                        uint8_t b = buf[i + 6];
                        float a = buf[i + 7] / 255.0f;
                        pixels.push_back({x, y, Color(r, g, b, a)});
                    }
                    holder->setBuffer(pixels, clear);
                }),
                jac::PropFlags::Enumerable);

            proto.defineProperty("clear",
                                 ff.newFunctionThis([](jac::ContextRef ctx,
                                                       jac::ValueWeak thisVal) {
                                     getOpaque(ctx, thisVal)->clear();
                                 }),
                                 jac::PropFlags::Enumerable);

            proto.defineProperty(
                "setBrightness",
                ff.newFunctionThis([](jac::ContextRef ctx,
                                      jac::ValueWeak thisVal, int brightness) {
                    getOpaque(ctx, thisVal)->setBrightness(brightness);
                }),
                jac::PropFlags::Enumerable);

            proto.defineProperty(
                "isInitialized",
                ff.newFunctionThis(
                    [](jac::ContextRef ctx, jac::ValueWeak thisVal) {
                        return jac::Value::from(
                            ctx, getOpaque(ctx, thisVal)->isInitialized());
                    }),
                jac::PropFlags::Enumerable);
        }
    };

    using Hub75Class = jac::Class<Hub75ProtoBuilder>;
    Hub75Feature() { Hub75Class::init("Hub75"); }

    void initialize() {
        Next::initialize();
        jac::Logger::debug("Hub75Feature: Initializing Hub75Feature module...");
        auto &mod = this->newModule("hub75");
        mod.addExport("Hub75", Hub75Class::getConstructor(this->context()));
    }

    ~Hub75Feature() {}
};

template <class Next> Hub75Holder *Hub75Feature<Next>::s_instance = nullptr;
inline Hub75Holder *Hub75Holder::active_instance = nullptr;
