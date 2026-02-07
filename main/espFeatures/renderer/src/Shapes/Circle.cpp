#include "Shapes/Circle.hpp"
#include "types.h"
#include <cmath>

Circle::Circle(const CircleParams &params)
    : Shape(params), radius(params.radius), fill(params.fill) {}

Collider *Circle::defaultCollider() { return new CircleCollider(x, y, radius); }

int Circle::getRadius() const { return radius; }

std::vector<std::pair<int, int>> Circle::getPointsToDraw(int cx, int cy, int x,
                                                         int y) {
    return {{cx + x, cy + y}, {cx - x, cy + y}, {cx + x, cy - y},
            {cx - x, cy - y}, {cx + y, cy + x}, {cx - y, cy + x},
            {cx + y, cy - x}, {cx - y, cy - x}};
}

void Circle::drawCirclePoints(Pixels &points, int cx, int cy, int x, int y) {
    auto pointsToDraw = getPointsToDraw(cx, cy, x, y);
    for (const auto &point : pointsToDraw) {
        Color sampledColor = sampleTexture(point.first, point.second);
        points.push_back(Pixel(static_cast<int>(std::round(point.first)),
                               static_cast<int>(std::round(point.second)),
                               sampledColor));
    }
}

void Circle::drawAntiAliasedPoint(Pixels &points, int cx, int cy, int x, int y,
                                  float intensity) {
    auto pointsToDraw = getPointsToDraw(cx, cy, x, y);
    for (const auto &point : pointsToDraw) {
        int clampedX = point.first;
        int clampedY = point.second;

        if (intensity > 0.01f && clampedX >= 0 && clampedY >= 0) {
            Color sampledColor = sampleTexture(point.first, point.second);
            points.push_back(Pixel(
                clampedX, clampedY,
                Color(sampledColor.r, sampledColor.g, sampledColor.b,
                      std::max(0.0f,
                               std::min(1.0f, intensity * sampledColor.a)))));
        }
    }
}

void Circle::fillCircle(Pixels &points, int cx, int cy, int r) {
    points.reserve(points.size() + 3.14159f * r * r);
    int x = 0;
    int y = r;
    int d = 3 - 2 * r;

    auto drawline = [&](int x1, int y1, int x2, int y2) {
        for (int i = x1; i <= x2; i++) {
            Color sampledColor = sampleTexture(i, y1);
            sampledColor.a = 1.0f;
            points.push_back(Pixel(i, y1, sampledColor));
        }
    };

    while (y >= x) {
        drawline(cx - x, cy + y, cx + x, cy + y);
        if (y != 0) {
            drawline(cx - x, cy - y, cx + x, cy - y);
        }
        if (x != y) {
            drawline(cx - y, cy + x, cx + y, cy + x);
            if (x != 0) {
                drawline(cx - y, cy - x, cx + y, cy - x);
            }
        }

        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
    }
}

void Circle::fillCircleAntiAliased(Pixels &points, int cx, int cy, int r) {
    points.reserve(points.size() + 3.14159f * r * r);
    int x = 0;
    int y = r;
    int d = 3 - 2 * r;

    auto drawline = [&](int x1, int y1, int x2, int y2) {
        for (int i = x1; i <= x2; i++) {
            Color sampledColor = sampleTexture(i, y1);
            float alpha = sampledColor.a;
            if (alpha > 0.01f) {
                points.push_back(Pixel(i, y1,
                                       Color(sampledColor.r, sampledColor.g,
                                             sampledColor.b, alpha)));
            }
        }
    };

    while (y >= x) {
        drawline(cx - x, cy + y, cx + x, cy + y);
        if (y != 0) {
            drawline(cx - x, cy - y, cx + x, cy - y);
        }
        if (x != y) {
            drawline(cx - y, cy + x, cx + y, cy + x);
            if (x != 0) {
                drawline(cx - y, cy - x, cx + y, cy - x);
            }
        }

        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
    }
}

void Circle::drawAliased(Pixels &pixels) {
    auto center = getTransformedPosition(x, y);
    int r = radius;

    int xPos = 0;
    int yPos = r;
    int d = 1 - r;

    while (xPos <= yPos) {
        drawCirclePoints(pixels, center.first, center.second, xPos, yPos);

        if (d < 0) {
            d = d + 2 * xPos + 3;
        } else {
            d = d + 2 * (xPos - yPos) + 5;
            yPos--;
        }
        xPos++;
    }

    if (fill) {
        fillCircle(pixels, center.first, center.second, r);
    }
}

void Circle::drawAntiAliased(Pixels &pixels) {
    auto center = getTransformedPosition(x, y);
    int r = radius;

    // Xiaolin Wu's circle algorithm for anti-aliased rendering
    float sqrt2 = std::sqrt(2.0f);
    float maxX = r / sqrt2;

    for (float xPos = 0; xPos <= maxX; xPos++) {
        float yPos = std::sqrt(r * r - xPos * xPos);

        float error = yPos - std::floor(yPos);
        float intensity = error;
        float intensity2 = 1 - error;

        float y1 = std::floor(yPos);
        float y2 = y1 + 1;

        drawAntiAliasedPoint(pixels, center.first, center.second, xPos, y1,
                             intensity2);
        drawAntiAliasedPoint(pixels, center.first, center.second, xPos, y2,
                             intensity);
        drawAntiAliasedPoint(pixels, center.first, center.second, y1, xPos,
                             intensity2);
        drawAntiAliasedPoint(pixels, center.first, center.second, y2, xPos,
                             intensity);
    }

    if (fill) {
        fillCircleAntiAliased(pixels, center.first, center.second, r);
    }
}
