#include "Shapes/Rectangle.hpp"
#include <algorithm>

Rectangle::Rectangle(const RectangleParams &params)
    : Shape(params), width(params.width), height(params.height),
      fill(params.fill) {}

Collider *Rectangle::defaultCollider() {
    return new RectangleCollider(x, y, width, height);
}

std::vector<std::pair<int, int>> Rectangle::getVertices() {
    std::vector<std::pair<int, int>> vertices;

    vertices.push_back(getTransformedPosition(x, y));              // TL
    vertices.push_back(getTransformedPosition(x, y + height - 1)); // BL
    vertices.push_back(
        getTransformedPosition(x + width - 1, y + height - 1));   // BR
    vertices.push_back(getTransformedPosition(x + width - 1, y)); // TR

    return vertices;
}

void Rectangle::getInsidePoints(
    Pixels &points, const std::vector<std::pair<int, int>> &vertices) {
    if (vertices.size() < 4)
        return;

    int minX =
        std::min(vertices[0].first,
                 std::min(vertices[1].first,
                          std::min(vertices[2].first, vertices[3].first)));
    int maxX =
        std::max(vertices[0].first,
                 std::max(vertices[1].first,
                          std::max(vertices[2].first, vertices[3].first)));
    int minY =
        std::min(vertices[0].second,
                 std::min(vertices[1].second,
                          std::min(vertices[2].second, vertices[3].second)));
    int maxY =
        std::max(vertices[0].second,
                 std::max(vertices[1].second,
                          std::max(vertices[2].second, vertices[3].second)));

    int v0x = vertices[0].first, v0y = vertices[0].second;
    int v1x = vertices[1].first, v1y = vertices[1].second;
    int v3x = vertices[3].first, v3y = vertices[3].second;

    int side1x = v1x - v0x;
    int side1y = v1y - v0y;

    int side2x = v3x - v0x;
    int side2y = v3y - v0y;

    int dot11 = side1x * side1x + side1y * side1y;
    int dot22 = side2x * side2x + side2y * side2y;
    int dot12 = side1x * side2x + side1y * side2y;
    float invDenom = 1.0f / (dot11 * dot22 - dot12 * dot12);

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            int pointVecX = x - v0x;
            int pointVecY = y - v0y;

            float dot1p = pointVecX * side1x + pointVecY * side1y;
            float dot2p = pointVecX * side2x + pointVecY * side2y;

            float u = (dot22 * dot1p - dot12 * dot2p) * invDenom;
            float v = (dot11 * dot2p - dot12 * dot1p) * invDenom;

            if (u >= 0.0f && u <= 1.0f && v >= 0.0f && v <= 1.0f) {
                points.push_back(Pixel(x, y, sampleTexture(x, y)));
            }
        }
    }
}
void Rectangle::drawAntiAliased(Pixels &pixels) {
    auto vertices = getVertices();

    if (vertices.size() >= 4) {
        auto tl = vertices[0];
        auto bl = vertices[1];
        auto br = vertices[2];
        auto tr = vertices[3];

        wuLine(pixels, tl.first, tl.second, tr.first, tr.second);
        wuLine(pixels, bl.first, bl.second, br.first, br.second);
        wuLine(pixels, tl.first, tl.second, bl.first, bl.second);
        wuLine(pixels, tr.first, tr.second, br.first, br.second);
    }

    if (fill) {
        getInsidePoints(pixels, vertices);
    }
}

void Rectangle::drawAliased(Pixels &pixels) {
    auto vertices = getVertices();

    if (vertices.size() >= 4) {
        auto tl = vertices[0];
        auto bl = vertices[1];
        auto br = vertices[2];
        auto tr = vertices[3];

        bresenhamLine(pixels, tl.first, tl.second, tr.first, tr.second);
        bresenhamLine(pixels, bl.first, bl.second, br.first, br.second);
        bresenhamLine(pixels, tl.first, tl.second, bl.first, bl.second);
        bresenhamLine(pixels, tr.first, tr.second, br.first, br.second);
    }

    if (fill) {
        getInsidePoints(pixels, vertices);
    }
}
