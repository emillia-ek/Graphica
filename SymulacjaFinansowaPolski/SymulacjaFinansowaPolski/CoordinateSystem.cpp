#include "CoordinateSystem.h"
#include <cmath>

using namespace std;

CoordinateSystem::CoordinateSystem() : viewXMin(-10.0f), viewXMax(10.0f), viewYMin(-10.0f), viewYMax(10.0f),
                                       baseXMin(-10.0f), baseXMax(10.0f), baseYMin(-10.0f), baseYMax(10.0f) {}

float CoordinateSystem::getSpacing(float range) {
    if (range > 50.0f) return 5.0f;
    else if (range > 20.0f) return 2.0f;
    else if (range < 2.0f) return 0.2f;
    else if (range < 5.0f) return 0.5f;
    return 1.0f;
}

void CoordinateSystem::drawArrows() {
    glColor3f(0.8f, 0.8f, 0.8f);

    float arrowSizeX = (viewXMax - viewXMin) * 0.015f;
    float arrowSizeY = (viewYMax - viewYMin) * 0.015f;

    glBegin(GL_TRIANGLES);

    glVertex2f(viewXMax, 0.0f);
    glVertex2f(viewXMax - arrowSizeX, arrowSizeY/2.0f);
    glVertex2f(viewXMax - arrowSizeX, -arrowSizeY/2.0f);

    glVertex2f(0.0f, viewYMax);
    glVertex2f(arrowSizeX/2.0f, viewYMax - arrowSizeY);
    glVertex2f(-arrowSizeX/2.0f, viewYMax - arrowSizeY);

    glEnd();
}

void CoordinateSystem::drawAxisLabels(float xSpacing, float ySpacing) {
    glColor3f(0.7f, 0.7f, 0.7f);

    int startX = static_cast<int>(ceil(viewXMin / xSpacing));
    int endX = static_cast<int>(floor(viewXMax / xSpacing));

    for (int x = startX; x <= endX; x++) {
        if (x != 0) {
            glBegin(GL_LINES);
            glVertex2f(x * xSpacing, -0.1f);
            glVertex2f(x * xSpacing, 0.1f);
            glEnd();
        }
    }

    int startY = static_cast<int>(ceil(viewYMin / ySpacing));
    int endY = static_cast<int>(floor(viewYMax / ySpacing));

    for (int y = startY; y <= endY; y++) {
        if (y != 0) {
            glBegin(GL_LINES);
            glVertex2f(-0.1f, y * ySpacing);
            glVertex2f(0.1f, y * ySpacing);
            glEnd();
        }
    }
}

void CoordinateSystem::draw(GLFWwindow* window) {
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    float aspect = (float)windowWidth / windowHeight;

    float currentWidth = viewXMax - viewXMin;
    float currentHeight = viewYMax - viewYMin;

    float centerX = (viewXMin + viewXMax) / 2.0f;
    float centerY = (viewYMin + viewYMax) / 2.0f;

    float targetWidth, targetHeight;

    if (aspect > 1.0f) {
        targetHeight = currentHeight;
        targetWidth = targetHeight * aspect;
    } else {
        targetWidth = currentWidth;
        targetHeight = targetWidth / aspect;
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(centerX - targetWidth/2, centerX + targetWidth/2,
            centerY - targetHeight/2, centerY + targetHeight/2, -1.0f, 1.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Draw grid
    glColor3f(0.12f, 0.12f, 0.12f);
    glBegin(GL_LINES);

    float xSpacing = getSpacing(targetWidth);
    float ySpacing = getSpacing(targetHeight);

    float xStart = ceil((centerX - targetWidth/2) / xSpacing) * xSpacing;
    float xEnd = centerX + targetWidth/2;
    for (float x = xStart; x <= xEnd; x += xSpacing) {
        if (fabs(x) > 0.0001f) {
            glVertex2f(x, centerY - targetHeight/2);
            glVertex2f(x, centerY + targetHeight/2);
        }
    }

    float yStart = ceil((centerY - targetHeight/2) / ySpacing) * ySpacing;
    float yEnd = centerY + targetHeight/2;
    for (float y = yStart; y <= yEnd; y += ySpacing) {
        if (fabs(y) > 0.0001f) {
            glVertex2f(centerX - targetWidth/2, y);
            glVertex2f(centerX + targetWidth/2, y);
        }
    }

    glEnd();

    // Draw thicker axes
    glColor3f(0.4f, 0.4f, 0.4f);
    glLineWidth(1.5f);
    glBegin(GL_LINES);

    // X axis
    glVertex2f(centerX - targetWidth/2, 0.0f);
    glVertex2f(centerX + targetWidth/2, 0.0f);

    // Y axis
    glVertex2f(0.0f, centerY - targetHeight/2);
    glVertex2f(0.0f, centerY + targetHeight/2);

    glEnd();
    glLineWidth(1.0f);

    drawArrows();
    drawAxisLabels(xSpacing, ySpacing);

    // Draw main unit lines slightly darker
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_LINES);

    // Main horizontal lines
    for (float y = yStart; y <= yEnd; y += ySpacing) {
        if (fabs(y) > 0.0001f) {
            glVertex2f(centerX - targetWidth/2, y);
            glVertex2f(centerX + targetWidth/2, y);
        }
    }

    // Main vertical lines
    for (float x = xStart; x <= xEnd; x += xSpacing) {
        if (fabs(x) > 0.0001f) {
            glVertex2f(x, centerY - targetHeight/2);
            glVertex2f(x, centerY + targetHeight/2);
        }
    }

    glEnd();
}

void CoordinateSystem::setViewRange(float xmin, float xmax, float ymin, float ymax) {
    viewXMin = xmin;
    viewXMax = xmax;
    viewYMin = ymin;
    viewYMax = ymax;
}

void CoordinateSystem::zoom(float factor, float centerX, float centerY) {
    float xRange = viewXMax - viewXMin;
    float yRange = viewYMax - viewYMin;

    float newXRange = xRange * factor;
    float newYRange = yRange * factor;

    viewXMin = centerX - newXRange / 2.0f;
    viewXMax = centerX + newXRange / 2.0f;
    viewYMin = centerY - newYRange / 2.0f;
    viewYMax = centerY + newYRange / 2.0f;
}

void CoordinateSystem::pan(float dx, float dy) {
    float xRange = viewXMax - viewXMin;
    float yRange = viewYMax - viewYMin;

    viewXMin += dx * xRange;
    viewXMax += dx * xRange;
    viewYMin += dy * yRange;
    viewYMax += dy * yRange;
}

void CoordinateSystem::resetView() {
    viewXMin = baseXMin;
    viewXMax = baseXMax;
    viewYMin = baseYMin;
    viewYMax = baseYMax;
}

void CoordinateSystem::getViewRange(float& xmin, float& xmax, float& ymin, float& ymax) const {
    xmin = viewXMin;
    xmax = viewXMax;
    ymin = viewYMin;
    ymax = viewYMax;
}

void CoordinateSystem::screenToGraph(GLFWwindow* window, int screenX, int screenY, float& graphX, float& graphY) {
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    float centerX = (viewXMin + viewXMax) / 2.0f;
    float centerY = (viewYMin + viewYMax) / 2.0f;
    float currentWidth = viewXMax - viewXMin;
    float currentHeight = viewYMax - viewYMin;

    float aspect = (float)width / height;
    float targetWidth, targetHeight;

    if (aspect > 1.0f) {
        targetHeight = currentHeight;
        targetWidth = targetHeight * aspect;
    } else {
        targetWidth = currentWidth;
        targetHeight = targetWidth / aspect;
    }

    graphX = centerX - targetWidth/2 + (screenX / (float)width) * targetWidth;
    graphY = centerY + targetHeight/2 - (screenY / (float)height) * targetHeight;
}