#include "types.h"

bool operator==(const Color &lhs, const Color &rhs) {
    return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b && lhs.a == rhs.a;
}

bool operator!=(const Color &lhs, const Color &rhs) {
    return !(lhs == rhs);
}

bool operator==(const Pixel &lhs, const Pixel &rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.color == rhs.color;
}

bool operator!=(const Pixel &lhs, const Pixel &rhs) {
    return !(lhs == rhs);
}
