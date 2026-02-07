#include "Shapes/Point.hpp"

Point::Point(const ShapeParams &params) : Shape(params) {}

Collider *Point::defaultCollider() { return new PointCollider(x, y); }

void Point::drawAntiAliased(Pixels &pixels) {
    Color sampledColor = sampleTexture(x, y);
    pixels.push_back(Pixel(x, y, sampledColor));
}

void Point::drawAliased(Pixels &pixels) {
    Color sampledColor = sampleTexture(x, y);
    pixels.push_back(Pixel(x, y, sampledColor));
}
