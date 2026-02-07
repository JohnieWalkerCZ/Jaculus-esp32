#pragma once
#include "Shape.hpp"

struct CircleParams : public ShapeParams {
    int radius;
    bool fill;

    CircleParams(int x, int y, const Color &color, int radius,
                 bool fill = false, int z = 0)
        : ShapeParams(x, y, color, z), radius(radius), fill(fill) {}
};

class Circle : public Shape {
  private:
    int radius;
    bool fill;

  public:
    Circle(const CircleParams &params);
    Collider *defaultCollider() override;
    void drawAntiAliased(Pixels &pixels) override;
    void drawAliased(Pixels &pixels) override;
    int getRadius() const;

  private:
    std::vector<std::pair<int, int>> getPointsToDraw(int cx, int cy, int x,
                                                     int y);
    void drawCirclePoints(Pixels &points, int cx, int cy, int x, int y);
    void drawAntiAliasedPoint(Pixels &points, int cx, int cy, int x, int y,
                              float intensity);
    void fillCircle(Pixels &points, int cx, int cy, int r);
    void fillCircleAntiAliased(Pixels &points, int cx, int cy, int r);
};
