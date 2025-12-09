#ifndef POINT_H
#define POINT_H

struct Point {
    float x, y;
    Point(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}
};

#endif