#include "Shapes/Polygon.hpp"

Polygon::Polygon(const PolygonParams &params)
    : Shape(params), vertices(params.vertices), fill(params.fill) {
    for (auto &vertex : vertices) {
        vertex.first += params.x;
        vertex.second += params.y;
    }
}

Collider *Polygon::defaultCollider() {
    return new PolygonCollider(x, y, vertices);
}

std::vector<std::pair<int, int>> Polygon::getTransformedVertices() {
    std::vector<std::pair<int, int>> transformed;
    for (const auto &v : vertices) {
        transformed.push_back(getTransformedPosition(v.first, v.second));
    }
    return transformed;
}

void Polygon::getInsidePoints(
    Pixels &points, const std::vector<std::pair<int, int>> &vertices) {
    if (vertices.size() < 3)
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
void Polygon::drawAliased(Pixels &pixels) {
    auto transformedVertices = getTransformedVertices();

    if (transformedVertices.size() >= 3) {
        for (size_t i = 0; i < transformedVertices.size(); i++) {
            size_t j = (i + 1) % transformedVertices.size();
            bresenhamLine(pixels, transformedVertices[i].first,
                          transformedVertices[i].second,
                          transformedVertices[j].first,
                          transformedVertices[j].second);
        }
    }

    if (fill) {
        getInsidePoints(pixels, transformedVertices);
    }
}

void Polygon::drawAntiAliased(Pixels &pixels) {
    auto transformedVertices = getTransformedVertices();

    if (transformedVertices.size() >= 3) {
        for (size_t i = 0; i < transformedVertices.size(); i++) {
            size_t j = (i + 1) % transformedVertices.size();
            wuLine(pixels, transformedVertices[i].first,
                   transformedVertices[i].second, transformedVertices[j].first,
                   transformedVertices[j].second);
        }
    }

    if (fill) {
        getInsidePoints(pixels, transformedVertices);
    }
}
void Polygon::getInsidePointsWithTexture(
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
