#pragma once
#include <cmath>
#include <cstring>
#include <vector>

class CircleCollider;
class RectangleCollider;
class PolygonCollider;
class LineSegmentCollider;
class PointCollider;
class RegularPolygonCollider;

enum class ColliderType {
    CIRCLE,
    RECTANGLE,
    POLYGON,
    LINE,
    POINT,
    REGULAR_POLYGON
};

class CollisionVisitor {
  public:
    virtual bool visitCircle(const CircleCollider *circle) = 0;
    virtual bool visitRectangle(const RectangleCollider *rect) = 0;
    virtual bool visitPolygon(const PolygonCollider *polygon) = 0;
    virtual bool visitLine(const LineSegmentCollider *line) = 0;
    virtual bool visitPoint(const PointCollider *point) = 0;
    virtual bool
    visitRegularPolygon(const RegularPolygonCollider *regularPolygon) = 0;
    virtual ~CollisionVisitor() = default;
};

class Collidable {
  public:
    virtual bool accept(CollisionVisitor *visitor) const = 0;
    virtual ColliderType getType() const = 0;
    virtual ~Collidable() = default;
};

class Collider : public Collidable {
  public:
    float x;
    float y;

    Collider(int x, int y) : x(x), y(y) {}
    virtual ~Collider() = default;

    virtual bool intersects(const Collider *other) const = 0;
    virtual void translate(float dx, float dy) {
        x += dx;
        y += dy;
    }
    virtual void translate(int dx, int dy) {
        x += dx;
        y += dy;
    }

    virtual void setPosition(int newX, int newY) {
        x = newX;
        y = newY;
    }

    void setX(int newX) { x = newX; }
    void setY(int newY) { y = newY; }

    int getX() const { return x; }
    int getY() const { return y; }
};

namespace CollisionMath {
inline int distanceSquared(int x1, int y1, int x2, int y2) {
    int dx = x1 - x2;
    int dy = y1 - y2;
    return dx * dx + dy * dy;
}

inline bool pointInPolygon(int x, int y,
                           const std::vector<std::pair<int, int>> &points) {
    bool inside = false;
    size_t n = points.size();
    for (size_t i = 0, j = n - 1; i < n; j = i++) {
        int xi = points[i].first, yi = points[i].second;
        int xj = points[j].first, yj = points[j].second;

        bool intersect =
            ((yi > y) != (yj > y)) &&
            (x < (float)(xj - xi) * (y - yi) / (float)(yj - yi) + xi);
        if (intersect)
            inside = !inside;
    }
    return inside;
}

inline bool lineIntersectLine(int x1, int y1, int x2, int y2, int x3, int y3,
                              int x4, int y4) {
    int denominator = ((y4 - y3) * (x2 - x1) - (x4 - x3) * (y2 - y1));

    if (denominator == 0)
        return false;

    float ua = ((x4 - x3) * (y1 - y3) - (y4 - y3) * (x1 - x3)) / denominator;
    float ub = ((x2 - x1) * (y1 - y3) - (y2 - y1) * (x1 - x3)) / denominator;

    return ua >= 0 && ua <= 1 && ub >= 0 && ub <= 1;
}
} // namespace CollisionMath

class IntersectionVisitor : public CollisionVisitor {
  private:
    const Collider *other;

  public:
    IntersectionVisitor(const Collider *other) : other(other) {}

    bool visitCircle(const CircleCollider *circle) override;
    bool visitRectangle(const RectangleCollider *rect) override;
    bool visitPolygon(const PolygonCollider *polygon) override;
    bool visitLine(const LineSegmentCollider *line) override;
    bool visitPoint(const PointCollider *point) override;
    bool
    visitRegularPolygon(const RegularPolygonCollider *regularPolygon) override;

  private:
    bool circleCircle(const CircleCollider *c1, const CircleCollider *c2);
    bool circleRectangle(const CircleCollider *circle,
                         const RectangleCollider *rect);
    bool circlePoint(const CircleCollider *circle, const PointCollider *point);
    bool circleLine(const CircleCollider *circle,
                    const LineSegmentCollider *line);
    bool circlePolygon(const CircleCollider *circle,
                       const PolygonCollider *polygon);
    bool circleRegularPolygon(const CircleCollider *circle,
                              const RegularPolygonCollider *regularPolygon);

    bool rectangleRectangle(const RectangleCollider *r1,
                            const RectangleCollider *r2);
    bool rectanglePoint(const RectangleCollider *rect,
                        const PointCollider *point);
    bool rectangleLine(const RectangleCollider *rect,
                       const LineSegmentCollider *line);
    bool rectanglePolygon(const RectangleCollider *rect,
                          const PolygonCollider *polygon);
    bool rectangleRegularPolygon(const RectangleCollider *rect,
                                 const RegularPolygonCollider *regularPolygon);

    bool polygonPoint(const PolygonCollider *polygon,
                      const PointCollider *point);
    bool polygonLine(const PolygonCollider *polygon,
                     const LineSegmentCollider *line);
    bool polygonPolygon(const PolygonCollider *p1, const PolygonCollider *p2);
    bool polygonRegularPolygon(const PolygonCollider *polygon,
                               const RegularPolygonCollider *regularPolygon);

    bool linePoint(const LineSegmentCollider *line, const PointCollider *point);
    bool lineLine(const LineSegmentCollider *l1, const LineSegmentCollider *l2);
    bool lineRegularPolygon(const LineSegmentCollider *line,
                            const RegularPolygonCollider *regularPolygon);

    bool pointPoint(const PointCollider *p1, const PointCollider *p2);
    bool pointRegularPolygon(const PointCollider *point,
                             const RegularPolygonCollider *regularPolygon);
};

class CircleCollider : public Collider {
  public:
    int radius;

    CircleCollider(int x, int y, int radius) : Collider(x, y), radius(radius) {}

    int getRadius() const { return radius; }
    void setRadius(int radius) { this->radius = radius; }

    bool accept(CollisionVisitor *visitor) const override {
        return visitor->visitCircle(this);
    }

    ColliderType getType() const override { return ColliderType::CIRCLE; }

    bool intersects(const Collider *other) const override {
        IntersectionVisitor visitor(other);
        return this->accept(&visitor);
    }
};

class RectangleCollider : public Collider {
  public:
    int width;
    int height;

    RectangleCollider(int x, int y, int width, int height)
        : Collider(x, y), width(width), height(height) {}

    int getWidth() const { return width; }
    int getHeight() const { return height; }
    void setWidth(int width) { this->width = width; }
    void setHeight(int height) { this->height = height; }

    bool accept(CollisionVisitor *visitor) const override {
        return visitor->visitRectangle(this);
    }

    ColliderType getType() const override { return ColliderType::RECTANGLE; }

    bool intersects(const Collider *other) const override {
        IntersectionVisitor visitor(other);
        return this->accept(&visitor);
    }
};

class PolygonCollider : public Collider {
  public:
    std::vector<std::pair<int, int>> points;

    PolygonCollider(int x, int y,
                    const std::vector<std::pair<int, int>> &points)
        : Collider(x, y), points(points) {}

    const std::vector<std::pair<int, int>> &getPoints() const { return points; }

    bool accept(CollisionVisitor *visitor) const override {
        return visitor->visitPolygon(this);
    }

    ColliderType getType() const override { return ColliderType::POLYGON; }

    bool intersects(const Collider *other) const override {
        IntersectionVisitor visitor(other);
        return this->accept(&visitor);
    }

    std::vector<std::pair<int, int>> getWorldPoints() const {
        std::vector<std::pair<int, int>> worldPoints;
        for (const auto &p : points) {
            worldPoints.push_back({p.first + x, p.second + y});
        }
        return worldPoints;
    }
};

class LineSegmentCollider : public Collider {
  public:
    int x2;
    int y2;

    LineSegmentCollider(int x1, int y1, int x2, int y2)
        : Collider(x1, y1), x2(x2), y2(y2) {}

    int getX2() const { return x2; }
    int getY2() const { return y2; }
    void setX2(int x2) { this->x2 = x2; }
    void setY2(int y2) { this->y2 = y2; }

    bool accept(CollisionVisitor *visitor) const override {
        return visitor->visitLine(this);
    }

    ColliderType getType() const override { return ColliderType::LINE; }

    bool intersects(const Collider *other) const override {
        IntersectionVisitor visitor(other);
        return this->accept(&visitor);
    }
};

class PointCollider : public Collider {
  public:
    PointCollider(int x, int y) : Collider(x, y) {}

    bool accept(CollisionVisitor *visitor) const override {
        return visitor->visitPoint(this);
    }

    ColliderType getType() const override { return ColliderType::POINT; }

    bool intersects(const Collider *other) const override {
        IntersectionVisitor visitor(other);
        return this->accept(&visitor);
    }
};

class RegularPolygonCollider : public Collider {
  public:
    int sides;
    int radius;

    RegularPolygonCollider(int x, int y, int sides, int radius)
        : Collider(x, y), sides(sides), radius(radius) {}

    int getSides() const { return sides; }
    int getRadius() const { return radius; }
    void setSides(int sides) { this->sides = sides; }
    void setRadius(int radius) { this->radius = radius; }

    bool accept(CollisionVisitor *visitor) const override {
        return visitor->visitRegularPolygon(this);
    }

    ColliderType getType() const override {
        return ColliderType::REGULAR_POLYGON;
    }

    bool intersects(const Collider *other) const override {
        IntersectionVisitor visitor(other);
        return this->accept(&visitor);
    }

    std::vector<std::pair<int, int>> generateRegularPolygonPoints() const {
        std::vector<std::pair<int, int>> points;
        for (int i = 0; i < sides; i++) {
            float angle = (i * 2 * M_PI / sides) - M_PI / 2;
            points.push_back({(int)std::round(std::cos(angle) * radius),
                              (int)std::round(std::sin(angle) * radius)});
        }
        return points;
    }
};
