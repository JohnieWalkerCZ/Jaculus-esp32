#include "Collider.hpp"
#include <algorithm>
#include <cmath>
#include <string>

bool IntersectionVisitor::visitCircle(const CircleCollider *circle) {
    switch (other->getType()) {
    case ColliderType::CIRCLE:
        return circleCircle(circle, static_cast<const CircleCollider *>(other));
    case ColliderType::RECTANGLE:
        return circleRectangle(circle,
                               static_cast<const RectangleCollider *>(other));
    case ColliderType::POINT:
        return circlePoint(circle, static_cast<const PointCollider *>(other));
    case ColliderType::LINE:
        return circleLine(circle,
                          static_cast<const LineSegmentCollider *>(other));
    case ColliderType::POLYGON:
        return circlePolygon(circle,
                             static_cast<const PolygonCollider *>(other));
    case ColliderType::REGULAR_POLYGON:
        return circleRegularPolygon(
            circle, static_cast<const RegularPolygonCollider *>(other));
    default:
        return false;
    }
}

bool IntersectionVisitor::visitRectangle(const RectangleCollider *rect) {
    switch (other->getType()) {
    case ColliderType::CIRCLE:
        return circleRectangle(static_cast<const CircleCollider *>(other),
                               rect);
    case ColliderType::RECTANGLE:
        return rectangleRectangle(
            rect, static_cast<const RectangleCollider *>(other));
    case ColliderType::POINT:
        return rectanglePoint(rect, static_cast<const PointCollider *>(other));
    case ColliderType::LINE:
        return rectangleLine(rect,
                             static_cast<const LineSegmentCollider *>(other));
    case ColliderType::POLYGON:
        return rectanglePolygon(rect,
                                static_cast<const PolygonCollider *>(other));
    case ColliderType::REGULAR_POLYGON:
        return rectangleRegularPolygon(
            rect, static_cast<const RegularPolygonCollider *>(other));
    default:
        return false;
    }
}

bool IntersectionVisitor::visitPolygon(const PolygonCollider *polygon) {
    switch (other->getType()) {
    case ColliderType::CIRCLE:
        return circlePolygon(static_cast<const CircleCollider *>(other),
                             polygon);
    case ColliderType::RECTANGLE:
        return rectanglePolygon(static_cast<const RectangleCollider *>(other),
                                polygon);
    case ColliderType::POINT:
        return polygonPoint(polygon, static_cast<const PointCollider *>(other));
    case ColliderType::LINE:
        return polygonLine(polygon,
                           static_cast<const LineSegmentCollider *>(other));
    case ColliderType::POLYGON:
        return polygonPolygon(polygon,
                              static_cast<const PolygonCollider *>(other));
    case ColliderType::REGULAR_POLYGON:
        return polygonRegularPolygon(
            polygon, static_cast<const RegularPolygonCollider *>(other));
    default:
        return false;
    }
}

bool IntersectionVisitor::visitLine(const LineSegmentCollider *line) {
    switch (other->getType()) {
    case ColliderType::CIRCLE:
        return circleLine(static_cast<const CircleCollider *>(other), line);
    case ColliderType::RECTANGLE:
        return rectangleLine(static_cast<const RectangleCollider *>(other),
                             line);
    case ColliderType::POINT:
        return linePoint(line, static_cast<const PointCollider *>(other));
    case ColliderType::LINE:
        return lineLine(line, static_cast<const LineSegmentCollider *>(other));
    case ColliderType::POLYGON:
        return polygonLine(static_cast<const PolygonCollider *>(other), line);
    case ColliderType::REGULAR_POLYGON:
        return lineRegularPolygon(
            line, static_cast<const RegularPolygonCollider *>(other));
    default:
        return false;
    }
}

bool IntersectionVisitor::visitPoint(const PointCollider *point) {
    switch (other->getType()) {
    case ColliderType::CIRCLE:
        return circlePoint(static_cast<const CircleCollider *>(other), point);
    case ColliderType::RECTANGLE:
        return rectanglePoint(static_cast<const RectangleCollider *>(other),
                              point);
    case ColliderType::POINT:
        return pointPoint(point, static_cast<const PointCollider *>(other));
    case ColliderType::LINE:
        return linePoint(static_cast<const LineSegmentCollider *>(other),
                         point);
    case ColliderType::POLYGON:
        return polygonPoint(static_cast<const PolygonCollider *>(other), point);
    case ColliderType::REGULAR_POLYGON:
        return pointRegularPolygon(
            point, static_cast<const RegularPolygonCollider *>(other));
    default:
        return false;
    }
}

bool IntersectionVisitor::visitRegularPolygon(
    const RegularPolygonCollider *regularPolygon) {
    CircleCollider circle(static_cast<int>(regularPolygon->x),
                          static_cast<int>(regularPolygon->y),
                          regularPolygon->radius);
    IntersectionVisitor circleVisitor(&circle);
    return other->accept(&circleVisitor);
}

bool IntersectionVisitor::circleCircle(const CircleCollider *c1,
                                       const CircleCollider *c2) {
    float distanceSquared =
        CollisionMath::distanceSquared(c1->x, c1->y, c2->x, c2->y);
    float radiusSum = c1->radius + c2->radius;
    return distanceSquared < radiusSum * radiusSum;
}

bool IntersectionVisitor::circleRectangle(const CircleCollider *circle,
                                          const RectangleCollider *rect) {
    float testX = circle->x;
    float testY = circle->y;

    if (circle->x < rect->x)
        testX = rect->x;
    else if (circle->x > rect->x + rect->width)
        testX = rect->x + rect->width;

    if (circle->y < rect->y)
        testY = rect->y;
    else if (circle->y > rect->y + rect->height)
        testY = rect->y + rect->height;

    float distX = circle->x - testX;
    float distY = circle->y - testY;
    float distance = sqrt((distX * distX) + (distY * distY));

    return distance <= circle->radius;
}
bool IntersectionVisitor::circlePoint(const CircleCollider *circle,
                                      const PointCollider *point) {
    float distanceSquared = CollisionMath::distanceSquared(circle->x, circle->y,
                                                           point->x, point->y);
    return distanceSquared <= (circle->radius * circle->radius);
}

bool IntersectionVisitor::circleLine(const CircleCollider *circle,
                                     const LineSegmentCollider *line) {
    float lineLength = std::sqrt((line->x2 - line->x) * (line->x2 - line->x) +
                                 (line->y2 - line->y) * (line->y2 - line->y));

    if (lineLength == 0) {
        PointCollider tempPoint(static_cast<int>(line->x),
                                static_cast<int>(line->y));
        return circlePoint(circle, &tempPoint);
    }

    float t = std::max(
        0.0f, std::min(1.0f, ((circle->x - line->x) * (line->x2 - line->x) +
                              (circle->y - line->y) * (line->y2 - line->y)) /
                                 (lineLength * lineLength)));

    float closestX = line->x + t * (line->x2 - line->x);
    float closestY = line->y + t * (line->y2 - line->y);

    float distanceSquared = CollisionMath::distanceSquared(circle->x, circle->y,
                                                           closestX, closestY);
    return distanceSquared < (circle->radius * circle->radius);
}

bool IntersectionVisitor::circlePolygon(const CircleCollider *circle,
                                        const PolygonCollider *polygon) {
    auto worldPoints = polygon->getWorldPoints();
    std::vector<std::pair<int, int>> pointsVec(worldPoints.begin(),
                                               worldPoints.end());

    if (CollisionMath::pointInPolygon(circle->x, circle->y, pointsVec)) {
        return true;
    }

    for (size_t i = 0; i < worldPoints.size(); i++) {
        size_t j = (i + 1) % worldPoints.size();
        LineSegmentCollider line(worldPoints[i].first, worldPoints[i].second,
                                 worldPoints[j].first, worldPoints[j].second);
        if (circleLine(circle, &line)) {
            return true;
        }
    }

    return false;
}

bool IntersectionVisitor::circleRegularPolygon(
    const CircleCollider *circle,
    const RegularPolygonCollider *regularPolygon) {
    CircleCollider otherCircle(static_cast<int>(regularPolygon->x),
                               static_cast<int>(regularPolygon->y),
                               regularPolygon->radius);
    return circleCircle(circle, &otherCircle);
}

bool IntersectionVisitor::rectangleRectangle(const RectangleCollider *r1,
                                             const RectangleCollider *r2) {
    int r1x = static_cast<int>(r1->x);
    int r1y = static_cast<int>(r1->y);
    int r2x = static_cast<int>(r2->x);
    int r2y = static_cast<int>(r2->y);

    return !(r1x >= r2x + r2->width ||  // R1 is right of R2
             r1x + r1->width <= r2x ||  // R1 is left of R2
             r1y >= r2y + r2->height || // R1 is below R2
             r1y + r1->height <= r2y);  // R1 is above R2
}
bool IntersectionVisitor::rectanglePoint(const RectangleCollider *rect,
                                         const PointCollider *point) {
    return point->x >= rect->x && point->x <= rect->x + rect->width &&
           point->y >= rect->y && point->y <= rect->y + rect->height;
}

bool IntersectionVisitor::rectangleLine(const RectangleCollider *rect,
                                        const LineSegmentCollider *line) {
    PointCollider startPoint(static_cast<int>(line->x),
                             static_cast<int>(line->y));
    PointCollider endPoint(static_cast<int>(line->x2),
                           static_cast<int>(line->y2));

    if (rectanglePoint(rect, &startPoint) || rectanglePoint(rect, &endPoint)) {
        return true;
    }

    LineSegmentCollider edges[4] = {
        {static_cast<int>(rect->x), static_cast<int>(rect->y),
         static_cast<int>(rect->x + rect->width), static_cast<int>(rect->y)},
        {static_cast<int>(rect->x + rect->width), static_cast<int>(rect->y),
         static_cast<int>(rect->x + rect->width),
         static_cast<int>(rect->y + rect->height)},
        {static_cast<int>(rect->x + rect->width),
         static_cast<int>(rect->y + rect->height), static_cast<int>(rect->x),
         static_cast<int>(rect->y + rect->height)},
        {static_cast<int>(rect->x), static_cast<int>(rect->y + rect->height),
         static_cast<int>(rect->x), static_cast<int>(rect->y)}};

    for (int i = 0; i < 4; i++) {
        if (lineLine(line, &edges[i])) {
            return true;
        }
    }

    return false;
}

bool IntersectionVisitor::rectanglePolygon(const RectangleCollider *rect,
                                           const PolygonCollider *polygon) {
    auto worldPoints = polygon->getWorldPoints();
    std::vector<std::pair<int, int>> pointsVec(worldPoints.begin(),
                                               worldPoints.end());

    for (const auto &point : worldPoints) {
        PointCollider tempPoint(point.first, point.second);
        if (rectanglePoint(rect, &tempPoint)) {
            return true;
        }
    }

    PointCollider corners[4] = {
        {static_cast<int>(rect->x), static_cast<int>(rect->y)},
        {static_cast<int>(rect->x + rect->width), static_cast<int>(rect->y)},
        {static_cast<int>(rect->x + rect->width),
         static_cast<int>(rect->y + rect->height)},
        {static_cast<int>(rect->x), static_cast<int>(rect->y + rect->height)}};

    for (int i = 0; i < 4; i++) {
        if (CollisionMath::pointInPolygon(corners[i].x, corners[i].y,
                                          pointsVec)) {
            return true;
        }
    }

    LineSegmentCollider rectEdges[4] = {
        {static_cast<int>(rect->x), static_cast<int>(rect->y),
         static_cast<int>(rect->x + rect->width), static_cast<int>(rect->y)},
        {static_cast<int>(rect->x + rect->width), static_cast<int>(rect->y),
         static_cast<int>(rect->x + rect->width),
         static_cast<int>(rect->y + rect->height)},
        {static_cast<int>(rect->x + rect->width),
         static_cast<int>(rect->y + rect->height), static_cast<int>(rect->x),
         static_cast<int>(rect->y + rect->height)},
        {static_cast<int>(rect->x), static_cast<int>(rect->y + rect->height),
         static_cast<int>(rect->x), static_cast<int>(rect->y)}};

    for (size_t i = 0; i < worldPoints.size(); i++) {
        size_t j = (i + 1) % worldPoints.size();
        LineSegmentCollider polyEdge(
            worldPoints[i].first, worldPoints[i].second, worldPoints[j].first,
            worldPoints[j].second);

        for (int k = 0; k < 4; k++) {
            if (lineLine(&rectEdges[k], &polyEdge)) {
                return true;
            }
        }
    }

    return false;
}

bool IntersectionVisitor::rectangleRegularPolygon(
    const RectangleCollider *rect,
    const RegularPolygonCollider *regularPolygon) {
    CircleCollider circle(static_cast<int>(regularPolygon->x),
                          static_cast<int>(regularPolygon->y),
                          regularPolygon->radius);
    return circleRectangle(&circle, rect);
}

bool IntersectionVisitor::polygonPoint(const PolygonCollider *polygon,
                                       const PointCollider *point) {
    auto worldPoints = polygon->getWorldPoints();
    std::vector<std::pair<int, int>> pointsVec(worldPoints.begin(),
                                               worldPoints.end());
    return CollisionMath::pointInPolygon(point->x, point->y, pointsVec);
}

bool IntersectionVisitor::polygonLine(const PolygonCollider *polygon,
                                      const LineSegmentCollider *line) {
    auto worldPoints = polygon->getWorldPoints();
    std::vector<std::pair<int, int>> pointsVec(worldPoints.begin(),
                                               worldPoints.end());

    PointCollider startPoint(static_cast<int>(line->x),
                             static_cast<int>(line->y));
    PointCollider endPoint(static_cast<int>(line->x2),
                           static_cast<int>(line->y2));

    if (polygonPoint(polygon, &startPoint) ||
        polygonPoint(polygon, &endPoint)) {
        return true;
    }

    for (size_t i = 0; i < worldPoints.size(); i++) {
        size_t j = (i + 1) % worldPoints.size();
        LineSegmentCollider edge(worldPoints[i].first, worldPoints[i].second,
                                 worldPoints[j].first, worldPoints[j].second);
        if (lineLine(line, &edge)) {
            return true;
        }
    }
    return false;
}

bool IntersectionVisitor::polygonPolygon(const PolygonCollider *p1,
                                         const PolygonCollider *p2) {
    auto worldPoints1 = p1->getWorldPoints();
    auto worldPoints2 = p2->getWorldPoints();
    std::vector<std::pair<int, int>> pointsVec1(worldPoints1.begin(),
                                                worldPoints1.end());
    std::vector<std::pair<int, int>> pointsVec2(worldPoints2.begin(),
                                                worldPoints2.end());

    for (const auto &point : worldPoints1) {
        if (CollisionMath::pointInPolygon(point.first, point.second,
                                          pointsVec2)) {
            return true;
        }
    }

    for (const auto &point : worldPoints2) {
        if (CollisionMath::pointInPolygon(point.first, point.second,
                                          pointsVec1)) {
            return true;
        }
    }

    for (size_t i = 0; i < worldPoints1.size(); i++) {
        size_t iNext = (i + 1) % worldPoints1.size();
        LineSegmentCollider edge1(worldPoints1[i].first, worldPoints1[i].second,
                                  worldPoints1[iNext].first,
                                  worldPoints1[iNext].second);

        for (size_t j = 0; j < worldPoints2.size(); j++) {
            size_t jNext = (j + 1) % worldPoints2.size();
            LineSegmentCollider edge2(
                worldPoints2[j].first, worldPoints2[j].second,
                worldPoints2[jNext].first, worldPoints2[jNext].second);

            if (lineLine(&edge1, &edge2)) {
                return true;
            }
        }
    }

    return false;
}

bool IntersectionVisitor::polygonRegularPolygon(
    const PolygonCollider *polygon,
    const RegularPolygonCollider *regularPolygon) {
    CircleCollider circle(static_cast<int>(regularPolygon->x),
                          static_cast<int>(regularPolygon->y),
                          regularPolygon->radius);
    return circlePolygon(&circle, polygon);
}

bool IntersectionVisitor::linePoint(const LineSegmentCollider *line,
                                    const PointCollider *point) {
    float crossProduct = (point->y - line->y) * (line->x2 - line->x) -
                         (point->x - line->x) * (line->y2 - line->y);
    if (std::abs(crossProduct) > 1e-10)
        return false;

    float dotProduct = (point->x - line->x) * (line->x2 - line->x) +
                       (point->y - line->y) * (line->y2 - line->y);
    if (dotProduct < 0)
        return false;

    float squaredLength = (line->x2 - line->x) * (line->x2 - line->x) +
                          (line->y2 - line->y) * (line->y2 - line->y);
    return dotProduct <= squaredLength;
}

bool IntersectionVisitor::lineLine(const LineSegmentCollider *l1,
                                   const LineSegmentCollider *l2) {
    return CollisionMath::lineIntersectLine(l1->x, l1->y, l1->x2, l1->y2, l2->x,
                                            l2->y, l2->x2, l2->y2);
}

bool IntersectionVisitor::lineRegularPolygon(
    const LineSegmentCollider *line,
    const RegularPolygonCollider *regularPolygon) {
    CircleCollider circle(static_cast<int>(regularPolygon->x),
                          static_cast<int>(regularPolygon->y),
                          regularPolygon->radius);
    return circleLine(&circle, line);
}

bool IntersectionVisitor::pointPoint(const PointCollider *p1,
                                     const PointCollider *p2) {
    return p1->x == p2->x && p1->y == p2->y;
}

bool IntersectionVisitor::pointRegularPolygon(
    const PointCollider *point, const RegularPolygonCollider *regularPolygon) {
    CircleCollider circle(static_cast<int>(regularPolygon->x),
                          static_cast<int>(regularPolygon->y),
                          regularPolygon->radius);
    return circlePoint(&circle, point);
}
