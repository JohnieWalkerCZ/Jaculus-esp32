#pragma once
#include "Shape.hpp"

struct RectangleParams : public ShapeParams {
    int width;
    int height;
    bool fill;

    RectangleParams(int x, int y, const Color &color, int width, int height,
                    bool fill = false, int z = 0)
        : ShapeParams(x, y, color, z), width(width), height(height),
          fill(fill) {}
};

class Rectangle : public Shape {
  private:
    int width;
    int height;
    bool fill;

  public:
    Rectangle(const RectangleParams &params);
    Collider *defaultCollider() override;
    void drawAntiAliased(Pixels &pixels) override;
    void drawAliased(Pixels &pixels) override;
    int getWidth() const { return width; }
    int getHeight() const { return height; }

  private:
    std::vector<std::pair<int, int>> getVertices();
    void getInsidePoints(Pixels &points,
                         const std::vector<std::pair<int, int>> &vertices);
};
