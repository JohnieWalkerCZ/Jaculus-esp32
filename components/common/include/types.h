#pragma once

#include <cstdint>
#include <vector>

struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    float a;

    Color() : r(0), g(0), b(0), a(1.0f) {}
    Color(uint8_t r, uint8_t g, uint8_t b, float a = 1.0f)
        : r(r), g(g), b(b), a(a) {}
};

struct Pixel {
    int x;
    int y;
    Color color;

    Pixel() : x(0), y(0), color() {}
    Pixel(int x, int y, Color color) : x(x), y(y), color(color) {}
};

using Pixels = std::vector<Pixel>;

namespace Colors {
    static const Color BLACK(0, 0, 0);
    static const Color WHITE(255, 255, 255);
    static const Color RED(255, 0, 0);
    static const Color GREEN(0, 255, 0);
    static const Color BLUE(0, 0, 255);
    static const Color YELLOW(255, 255, 0);
    static const Color MAGENTA(255, 0, 255);
    static const Color CYAN(0, 255, 255);
}

bool operator==(const Color &lhs, const Color &rhs);
bool operator!=(const Color &lhs, const Color &rhs);
bool operator==(const Pixel &lhs, const Pixel &rhs);
bool operator!=(const Pixel &lhs, const Pixel &rhs);

