#include "Shapes/LineSegment.hpp"
#include "esp_timer.h"
#include "jac/device/logger.h"
#include <cstdint>

LineSegment::LineSegment(const LineSegmentParams &params)
    : Shape(params), x2(params.x2), y2(params.y2) {}

Collider *LineSegment::defaultCollider() {
    return new LineSegmentCollider(x, y, x2, y2);
}

void LineSegment::drawAliased(Pixels &pixels) {
    auto transformedStart = getTransformedPosition(x, y);
    auto transformedEnd = getTransformedPosition(x2, y2);
    int x0 = transformedStart.first;
    int y0 = transformedStart.second;
    int x1 = transformedEnd.first;
    int y1 = transformedEnd.second;

    bresenhamLine(pixels, x0, y0, x1, y1);
}

void LineSegment::drawAntiAliased(Pixels &pixels) {
    auto transformedStart = getTransformedPosition(x, y);
    auto transformedEnd = getTransformedPosition(x2, y2);
    int x0 = transformedStart.first;
    int y0 = transformedStart.second;
    int x1 = transformedEnd.first;
    int y1 = transformedEnd.second;

    wuLine(pixels, x0, y0, x1, y1);
}
