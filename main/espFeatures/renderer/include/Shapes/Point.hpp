#pragma once
#include "Shape.hpp"

class Point : public Shape {
  public:
    Point(const ShapeParams &params);
    Collider *defaultCollider() override;
    void drawAntiAliased(Pixels &pixels) override;
    void drawAliased(Pixels &pixels) override;
};
