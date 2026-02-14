#include "Shapes/Collection.hpp"
#include <algorithm>

Collection::Collection(const ShapeParams &params) : Shape(params) {}

Collection::~Collection() {
    for (auto shape : shapes) {
        shape->setParent(nullptr);
    }
    shapes.clear();
}

Collider *Collection::defaultCollider() { return new CircleCollider(0, 0, 0); }

void Collection::addShape(Shape *shape) {
    shape->setParent(this);
    shapes.push_back(shape);
}

void Collection::drawAntiAliased(Pixels &pixels) {
    std::vector<Shape *> sortedShapes = shapes;
    std::sort(sortedShapes.begin(), sortedShapes.end(),
              [](Shape *a, Shape *b) { return a->getZ() < b->getZ(); });

    for (Shape *shape : sortedShapes) {
        shape->drawAntiAliased(pixels);
    }
}

void Collection::drawAliased(Pixels &pixels) {
    std::vector<Shape *> sortedShapes = shapes;
    std::sort(sortedShapes.begin(), sortedShapes.end(),
              [](Shape *a, Shape *b) { return a->getZ() < b->getZ(); });

    for (Shape *shape : sortedShapes) {
        shape->drawAliased(pixels);
    }
}

void Collection::clear() { shapes.clear(); }
