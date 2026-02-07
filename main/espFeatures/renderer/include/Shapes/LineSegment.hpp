#pragma once
#include "Shape.hpp"

struct LineSegmentParams : public ShapeParams {
    int x2;
    int y2;

    LineSegmentParams(int x, int y, const Color &color, int x2, int y2,
                      int z = 0)
        : ShapeParams(x, y, color, z), x2(x2), y2(y2) {}
};

class LineSegment : public Shape {
  private:
    int x2;
    int y2;

  public:
    LineSegment(const LineSegmentParams &params);
    Collider *defaultCollider() override;
    void drawAntiAliased(Pixels &pixels) override;
    void drawAliased(Pixels &pixels) override;
};
