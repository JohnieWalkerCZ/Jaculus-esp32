#pragma once
#include "LineSegment.hpp"
#include "Shape.hpp"
#include <vector>

struct PolygonParams : public ShapeParams {
    std::vector<std::pair<int, int>> vertices;
    bool fill;

    PolygonParams(int x, int y, const Color &color,
                  const std::vector<std::pair<int, int>> &vertices,
                  bool fill = false, int z = 0)
        : ShapeParams(x, y, color, z), vertices(vertices), fill(fill) {}
};

class Polygon : public Shape {
  private:
    std::vector<std::pair<int, int>> vertices;
    bool fill;

  public:
    Polygon(const PolygonParams &params);
    Collider *defaultCollider() override;
    void drawAntiAliased(Pixels &pixels) override;
    void drawAliased(Pixels &pixels) override;

  private:
    std::vector<std::pair<int, int>> getTransformedVertices();
    std::vector<LineSegment *> getSegments();
    void getInsidePoints(Pixels &points,
                         const std::vector<std::pair<int, int>> &vertices);
    void getInsidePointsWithTexture(
        Pixels &points, const std::vector<std::pair<int, int>> &vertices);
};
