#ifndef COORDINATESYSTEM_H
#define COORDINATESYSTEM_H

#include <GLFW/glfw3.h>

class CoordinateSystem {
private:
    float viewXMin, viewXMax, viewYMin, viewYMax;
    float baseXMin, baseXMax, baseYMin, baseYMax;

    float getSpacing(float range);
    void drawArrows();
    void drawAxisLabels(float xSpacing, float ySpacing);

public:
    CoordinateSystem();
    void draw(GLFWwindow* window);  // CHANGED: Take window as parameter
    void setViewRange(float xmin, float xmax, float ymin, float ymax);
    void zoom(float factor, float centerX, float centerY);
    void pan(float dx, float dy);
    void resetView();
    void getViewRange(float& xmin, float& xmax, float& ymin, float& ymax) const;
    void screenToGraph(GLFWwindow* window, int screenX, int screenY, float& graphX, float& graphY);  // CHANGED
};

#endif