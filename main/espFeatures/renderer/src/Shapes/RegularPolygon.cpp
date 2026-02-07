#include <algorithm>
#include <algorithm>
#include <algorithm>
#include "Shapes/RegularPolygon.hpp"
#include <cmath>

RegularPolygon::RegularPolygon(const RegularPolygonSideParams &params)
    : Shape(params), sides(params.sides), useSideLength(true),
      sideLength(params.sideLength), radius(0), fill(params.fill) {}

RegularPolygon::RegularPolygon(const RegularPolygonRadiusParams &params)
    : Shape(params), sides(params.sides), useSideLength(false), sideLength(0),
      radius(params.radius), fill(params.fill) {}

int RegularPolygon::getSides() const { return sides; }
int RegularPolygon::getRadius() {
    if (useSideLength) {
        return calculateRadiusFromSideLength(sideLength);
    }
    return radius;
}

void RegularPolygon::setSides(int sides) { this->sides = sides; }

void RegularPolygon::setRadius(int radius) {
    this->radius = radius;
    this->useSideLength = false;
}

int RegularPolygon::calculateRadiusFromSideLength(int sideLength) {
    return sideLength / (2 * std::sin(M_PI / sides));
}
void RegularPolygon::getInsidePoints(
    Pixels &points, const std::vector<std::pair<int, int>> &vertices) {
    if (vertices.size() < 3)
        return;

    int minX = vertices[0].first;
    int maxX = vertices[0].first;
    int minY = vertices[0].second;
    int maxY = vertices[0].second;

    for (size_t i = 1; i < vertices.size(); ++i) {
        const auto &v = vertices[i];
        if (v.first < minX)
            minX = v.first;
        if (v.first > maxX)
            maxX = v.first;
        if (v.second < minY)
            minY = v.second;
        if (v.second > maxY)
            maxY = v.second;
    }

    points.reserve(points.size() + (maxX - minX + 1) * (maxY - minY + 1) / 2);

    struct Edge {
        int x1, y1, x2, y2;
        float slope;
        bool operator<(const Edge &other) const { return y1 < other.y1; }
    };

    std::vector<Edge> edges;
    edges.reserve(vertices.size());

    size_t n = vertices.size();
    for (size_t i = 0; i < n; i++) {
        size_t j = (i + 1) % n;
        int x1 = vertices[i].first, y1 = vertices[i].second;
        int x2 = vertices[j].first, y2 = vertices[j].second;

        if (y1 != y2) {
            edges.push_back(
                {x1, y1, x2, y2, static_cast<float>(x2 - x1) / (y2 - y1)});
        }
    }

    for (int y = minY; y <= maxY; y++) {
        std::vector<int> intersections;
        intersections.reserve(edges.size());

        for (const auto &edge : edges) {
            if ((y >= edge.y1 && y < edge.y2) ||
                (y >= edge.y2 && y < edge.y1)) {
                float x = edge.x1 + edge.slope * (y - edge.y1);
                intersections.push_back(static_cast<int>(std::round(x)));
            }
        }

        std::sort(intersections.begin(), intersections.end());

        for (size_t i = 0; i < intersections.size(); i += 2) {
            if (i + 1 < intersections.size()) {
                int startX = std::max(minX, intersections[i]);
                int endX = std::min(maxX, intersections[i + 1]);

                for (int x = startX; x <= endX; x++) {
                    Color sampledColor = sampleTexture(x, y);
                    points.push_back(Pixel(x, y, sampledColor));
                }
            }
        }
    }
}
Collider *RegularPolygon::defaultCollider() {
    int effectiveRadius =
        useSideLength ? calculateRadiusFromSideLength(sideLength) : radius;
    return new RegularPolygonCollider(x, y, sides, effectiveRadius);
}

std::vector<std::pair<int, int>> RegularPolygon::getVertices() {
    int effectiveRadius;
    if (useSideLength) {
        effectiveRadius = calculateRadiusFromSideLength(sideLength);
    } else {
        effectiveRadius = radius;
    }

    std::vector<std::pair<int, int>> vertices;
    for (int i = 0; i < sides; i++) {
        float angle = 2 * M_PI / sides * i - M_PI / 2;
        float vx = x + effectiveRadius * std::cos(angle);
        float vy = y + effectiveRadius * std::sin(angle);
        auto rotated = getTransformedPosition(vx, vy);
        vertices.push_back(rotated);
    }
    return vertices;
}

void RegularPolygon::drawAliased(Pixels &pixels) {
    auto vertices = getVertices();

    if (vertices.size() >= 3) {
        for (size_t i = 0; i < vertices.size(); i++) {
            size_t j = (i + 1) % vertices.size();
            bresenhamLine(pixels, vertices[i].first, vertices[i].second,
                          vertices[j].first, vertices[j].second);
        }
    }

    if (fill) {
        getInsidePoints(pixels, vertices);
    }
}

void RegularPolygon::drawAntiAliased(Pixels &pixels) {
    auto vertices = getVertices();

    if (vertices.size() >= 3) {
        for (size_t i = 0; i < vertices.size(); i++) {
            size_t j = (i + 1) % vertices.size();
            wuLine(pixels, vertices[i].first, vertices[i].second,
                   vertices[j].first, vertices[j].second);
        }
    }

    if (fill) {
        getInsidePoints(pixels, vertices);
    }
}

void RegularPolygon::getInsidePointsWithTexture(
    Pixels &points, const std::vector<std::pair<int, int>> &vertices) {
    if (vertices.empty())
        return;

    int minX = vertices[0].first;
    int maxX = vertices[0].first;
    int minY = vertices[0].second;
    int maxY = vertices[0].second;

    for (const auto &v : vertices) {
        minX = std::min(minX, v.first);
        maxX = std::max(maxX, v.first);
        minY = std::min(minY, v.second);
        maxY = std::max(maxY, v.second);
    }

    for (int x = static_cast<int>(minX); x <= static_cast<int>(maxX); x++) {
        for (int y = static_cast<int>(minY); y <= static_cast<int>(maxY); y++) {
            bool inside = false;
            size_t n = vertices.size();
            for (size_t i = 0, j = n - 1; i < n; j = i++) {
                int xi = vertices[i].first, yi = vertices[i].second;
                int xj = vertices[j].first, yj = vertices[j].second;

                bool intersect = ((yi > y) != (yj > y)) &&
                                 (x < (xj - xi) * (y - yi) / (yj - yi) + xi);
                if (intersect)
                    inside = !inside;
            }

            if (inside) {
                Color sampledColor = sampleTexture(x, y);
                points.push_back(Pixel(x, y, sampledColor));
            }
        }
    }
}
