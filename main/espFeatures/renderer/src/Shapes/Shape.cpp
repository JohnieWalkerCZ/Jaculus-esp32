#include "Shapes/Shape.hpp"
#include "jac/device/logger.h"
#include <algorithm>
#include <cmath>

// Constructor & Destructor
Shape::Shape(const ShapeParams &params)
    : x(params.x), y(params.y), color(params.color), z(params.z),
      parent(nullptr), texture(nullptr), fixTexture(false), collider(nullptr) {
    rotation = {static_cast<int>(params.x), static_cast<int>(params.y), 0.0f};
    scale = {1.0f, 1.0f, static_cast<int>(params.x),
             static_cast<int>(params.y)};
    uvTransform = {1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f};
}

Shape::~Shape() {
    /*
    if (collider) {
        delete collider;
    }
    */
}

// Cache management
void Shape::updateTrigCache() {
    if (!trigCacheValid) {
        float angleRad = rotation.angle * M_PI / 180.0f;
        cosAngle = std::cos(angleRad);
        sinAngle = std::sin(angleRad);
        trigCacheValid = true;
    }
}

void Shape::updateTextureTrigCache() {
    if (!texTrigCacheValid) {
        float angleRad = uvTransform.rotation * M_PI / 180.0f;
        texCos = std::cos(angleRad);
        texSin = std::sin(angleRad);
        texTrigCacheValid = true;
    }
}

// Collider methods
void Shape::addCollider(Collider *collider) {
    if (collider) {
        this->collider = collider;
        return;
    }
    this->collider = defaultCollider();
}

void Shape::removeCollider() {
    if (collider) {
        delete collider;
        collider = nullptr;
    }
}

bool Shape::intersects(Shape *other) {
    if (this->collider && other->collider) {
        return this->collider->intersects(other->collider);
    }
    return false;
}

// Position and transformation methods
void Shape::setPosition(int x, int y) {
    this->x = x;
    this->y = y;
    if (collider) {
        collider->setPosition(x, y);
    }
}

void Shape::translate(int dx, int dy) {
    x += dx;
    y += dy;
    if (collider) {
        collider->translate(dx, dy);
    }
}

void Shape::translate(float dx, float dy) {
    x += dx;
    y += dy;
    if (collider) {
        collider->translate(dx, dy);
    }
}

void Shape::rotate(float angle) {
    rotation.angle = std::fmod(rotation.angle + angle, 360.0f);
    invalidateTrigCache();
}

void Shape::setPivot(int x, int y) {
    rotation.x = x;
    rotation.y = y;
}

void Shape::setZ(int z) { this->z = z; }

// Scaling methods
void Shape::setScale(float scaleX, float scaleY, float originX, float originY) {
    scale.x = scaleX;
    scale.y = scaleY;
    if (originX != -1)
        scale.originX = originX;
    if (originY != -1)
        scale.originY = originY;
}

void Shape::scaleX(float scaleX, float originX) {
    scale.x = scaleX;
    if (originX != -1)
        scale.originX = originX;
}

void Shape::scaleY(float scaleY, float originY) {
    scale.y = scaleY;
    if (originY != -1)
        scale.originY = originY;
}

void Shape::setScaleOrigin(int x, int y) {
    scale.originX = x;
    scale.originY = y;
}

// Texture methods
void Shape::setTexture(Texture *texture) { this->texture = texture; }

void Shape::setTextureScale(float scaleX, float scaleY) {
    uvTransform.scaleX = scaleX;
    uvTransform.scaleY = scaleY;
    uvTransform.invScaleX = 1.0f / scaleX;
    uvTransform.invScaleY = 1.0f / scaleY;
}

void Shape::setTextureOffset(float offsetX, float offsetY) {
    uvTransform.offsetX = offsetX;
    uvTransform.offsetY = offsetY;
}

void Shape::setTextureRotation(float rotation) {
    uvTransform.rotation = rotation;
    invalidateTexTrigCache();
}

void Shape::setFixTexture(bool fixed) { fixTexture = fixed; }

Color Shape::sampleTexture(int x, int y) {
    if (!texture)
        return color;

    int localX = x - this->x;
    int localY = y - this->y;

    if (fixTexture && rotation.angle != 0) {
        updateTrigCache();

        int rotationOriginX = rotation.x;
        int rotationOriginY = rotation.y;
        int originOffsetX = rotationOriginX - this->x;
        int originOffsetY = rotationOriginY - this->y;

        int translatedX = localX - originOffsetX;
        int translatedY = localY - originOffsetY;
        localX =
            translatedX * cosAngle - translatedY * sinAngle + originOffsetX;
        localY =
            translatedX * sinAngle + translatedY * cosAngle + originOffsetY;
    }
    int u =
        static_cast<int>(localX * uvTransform.invScaleX + uvTransform.offsetX);
    int v =
        static_cast<int>(localY * uvTransform.invScaleY + uvTransform.offsetY);
    if (uvTransform.rotation != 0) {
        updateTextureTrigCache();

        float texTranslatedU = u - 0.5f;
        float texTranslatedV = v - 0.5f;

        u = static_cast<int>(texTranslatedU * texCos - texTranslatedV * texSin +
                             0.5f);
        v = static_cast<int>(texTranslatedU * texSin + texTranslatedV * texCos +
                             0.5f);
    }
    Color texColor = texture->sample(u, v);
    texColor.a = texColor.a * color.a;
    return texColor;
}

// Parent-child relationship
void Shape::setParent(Shape *parent) { this->parent = parent; }

// Transformation and coordinate methods
std::pair<int, int> Shape::getTransformedPosition(int x, int y) {
    int currentX = x;
    int currentY = y;

    int scaleOriginX = scale.originX;
    int scaleOriginY = scale.originY;

    currentX = (currentX - scaleOriginX) * scale.x + scaleOriginX;
    currentY = (currentY - scaleOriginY) * scale.y + scaleOriginY;
    if (rotation.angle != 0) {
        updateTrigCache();

        int rotationOriginX = rotation.x;
        int rotationOriginY = rotation.y;

        int translatedX = currentX - rotationOriginX;
        int translatedY = currentY - rotationOriginY;

        currentX =
            translatedX * cosAngle + translatedY * sinAngle + rotationOriginX;
        currentY =
            -translatedX * sinAngle + translatedY * cosAngle + rotationOriginY;
    }

    if (parent) {
        auto parentTransformed =
            parent->getTransformedPosition(currentX, currentY);
        currentX = parentTransformed.first;
        currentY = parentTransformed.second;
    }

    return {std::round(currentX), std::round(currentY)};
}

// Drawing methods
void Shape::draw(Pixels &pixels, const DrawOptions &options) {
    options.antialias ? drawAntiAliased(pixels) : drawAliased(pixels);
}

// Line drawing algorithms
void Shape::bresenhamLine(Pixels &points, int x0, int y0, int x1, int y1) {
    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    points.reserve(points.size() + dx + dy + 1);

    int x = x0;
    int y = y0;

    Color sampledColor = texture ? sampleTexture(x, y) : color;
    points.push_back(Pixel(x, y, sampledColor));

    while (x != x1 || y != y1) {
        int e2 = err * 2;

        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }

        if (e2 < dx) {
            err += dx;
            y += sy;
        }

        if (texture) {
            sampledColor = sampleTexture(x, y);
        }
        points.push_back(Pixel(x, y, sampledColor));
    }
}

void Shape::wuLine(Pixels &points, int x0_int, int y0_int, int x1_int,
                   int y1_int) {
    float x0 = static_cast<float>(x0_int);
    float y0 = static_cast<float>(y0_int);
    float x1 = static_cast<float>(x1_int);
    float y1 = static_cast<float>(y1_int);

    bool steep = std::abs(y1 - y0) > std::abs(x1 - x0);

    if (steep) {
        std::swap(x0, y0);
        std::swap(x1, y1);
    }

    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    const float dx = x1 - x0;
    const float dy = y1 - y0;
    const float gradient = (dx == 0) ? 1.0f : dy / dx;

    const float x0_plus_half = x0 + 0.5f;
    const float x1_plus_half = x1 + 0.5f;

    int xpxl1 = static_cast<int>(std::round(x0));
    float yend = y0 + gradient * (xpxl1 - x0);
    float xgap =
        1.0f - (x0_plus_half - std::floor(x0_plus_half)); // rfpart(x0 + 0.5f)

    int ypxl1 = static_cast<int>(std::floor(yend));
    float yend_fpart = yend - ypxl1;
    float yend_rfpart = 1.0f - yend_fpart;

    if (steep) {
        addPixel(points, ypxl1, xpxl1, yend_rfpart * xgap);
        addPixel(points, ypxl1 + 1, xpxl1, yend_fpart * xgap);
    } else {
        addPixel(points, xpxl1, ypxl1, yend_rfpart * xgap);
        addPixel(points, xpxl1, ypxl1 + 1, yend_fpart * xgap);
    }

    float intery = yend + gradient;

    int xpxl2 = static_cast<int>(std::round(x1));
    yend = y1 + gradient * (xpxl2 - x1);
    xgap = x1_plus_half - std::floor(x1_plus_half); // fpart(x1 + 0.5f)

    int ypxl2 = static_cast<int>(std::floor(yend));
    yend_fpart = yend - ypxl2;
    yend_rfpart = 1.0f - yend_fpart;

    if (steep) {
        addPixel(points, ypxl2, xpxl2, yend_rfpart * xgap);
        addPixel(points, ypxl2 + 1, xpxl2, yend_fpart * xgap);
    } else {
        addPixel(points, xpxl2, ypxl2, yend_rfpart * xgap);
        addPixel(points, xpxl2, ypxl2 + 1, yend_fpart * xgap);
    }

    if (steep) {
        for (int x = xpxl1 + 1; x < xpxl2; x++) {
            int y_base = static_cast<int>(std::floor(intery));
            float intery_fpart = intery - y_base;
            float intery_rfpart = 1.0f - intery_fpart;

            addPixel(points, y_base, x, intery_rfpart);
            addPixel(points, y_base + 1, x, intery_fpart);
            intery += gradient;
        }
    } else {
        for (int x = xpxl1 + 1; x < xpxl2; x++) {
            int y_base = static_cast<int>(std::floor(intery));
            float intery_fpart = intery - y_base;
            float intery_rfpart = 1.0f - intery_fpart;

            addPixel(points, x, y_base, intery_rfpart);
            addPixel(points, x, y_base + 1, intery_fpart);
            intery += gradient;
        }
    }
}

// Pixel manipulation
void Shape::addPixel(Pixels &points, int x, int y, float alpha) {
    if (alpha < 0.01f)
        return;

    if (x < 0 || y < 0)
        return;

    float finalAlpha = std::max(0.0f, std::min(1.0f, alpha));

    Color pixelColor;

    if (!texture) {
        pixelColor = Color(color.r, color.g, color.b, finalAlpha * color.a);
    } else {
        Color texColor = sampleTexture(x, y);
        pixelColor =
            Color(texColor.r, texColor.g, texColor.b, finalAlpha * texColor.a);
    }

    points.push_back(Pixel(x, y, pixelColor));
}

// Fill algorithms
void Shape::getInsidePoints(Pixels &points,
                            const std::vector<std::pair<int, int>> &vertices) {
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

    for (int x = minX; x <= maxX; x++) {
        for (int y = minY; y <= maxY; y++) {
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
                points.push_back(Pixel(x, y, color));
            }
        }
    }
}
