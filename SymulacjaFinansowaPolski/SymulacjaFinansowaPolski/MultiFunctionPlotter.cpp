#include "MultiFunctionPlotter.h"
#include "MathExpressionParser.h"
#include <GLFW/glfw3.h>
#include <cmath>

using namespace std;

MultiFunctionPlotter::MultiFunctionPlotter() : xMin(-10.0f), xMax(10.0f), resolution(800), nextColorIndex(0) {
    colorPalette = {
        ImVec4(0.0f, 0.8f, 1.0f, 1.0f),
        ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
        ImVec4(0.3f, 1.0f, 0.3f, 1.0f),
        ImVec4(1.0f, 0.8f, 0.0f, 1.0f),
        ImVec4(0.8f, 0.3f, 1.0f, 1.0f),
        ImVec4(0.0f, 1.0f, 0.8f, 1.0f),
        ImVec4(1.0f, 0.5f, 0.0f, 1.0f),
        ImVec4(0.5f, 0.5f, 1.0f, 1.0f)
    };
}

void MultiFunctionPlotter::addFunction(const string& equation) {
    ImVec4 color = colorPalette[nextColorIndex % colorPalette.size()];
    functions.emplace_back(equation, color);
    nextColorIndex++;
    updateFunction(functions.size() - 1);
}

void MultiFunctionPlotter::editFunction(int index, const string& newEquation) {
    if (index >= 0 && index < functions.size()) {
        functions[index].expression = newEquation;
        updateFunction(index);
    }
}

void MultiFunctionPlotter::removeFunction(int index) {
    if (index >= 0 && index < functions.size()) {
        functions.erase(functions.begin() + index);
    }
}

void MultiFunctionPlotter::updateFunction(int index) {
    if (index < 0 || index >= functions.size()) return;

    auto& func = functions[index];
    func.points.clear();

    MathExpressionParser parser;
    parser.setExpression(func.expression);

    if (parser.getType() == VERTICAL_LINE) {
        float xValue = parser.getVerticalLineX();
        if (!isnan(xValue)) {
            for (float y = -100.0f; y <= 100.0f; y += 0.1f) {
                func.points.emplace_back(xValue, y);
            }
        }
        return;
    }

    if (parser.getType() == HORIZONTAL_LINE) {
        float yValue = parser.getHorizontalLineY();
        if (!isnan(yValue)) {
            for (float x = xMin; x <= xMax; x += 0.1f) {
                func.points.emplace_back(x, yValue);
            }
        }
        return;
    }

    if (parser.isCircleEquation()) {
        float cx, cy, r;
        parser.getCircleParams(cx, cy, r);

        int circlePoints = 200;
        for (int i = 0; i <= circlePoints; i++) {
            float angle = 2.0f * M_PI * i / circlePoints;
            float x = cx + r * cos(angle);
            float y = cy + r * sin(angle);
            func.points.emplace_back(x, y);
        }
        return;
    }

    float step = (xMax - xMin) / resolution;
    vector<Point> segmentPoints;

    for (int i = 0; i <= resolution; ++i) {
        float x = xMin + i * step;
        float y = parser.evaluate(x);

        if (!isnan(y) && !isinf(y)) {
            segmentPoints.emplace_back(x, y);
        } else {
            if (!segmentPoints.empty()) {
                func.points.insert(func.points.end(), segmentPoints.begin(), segmentPoints.end());
                segmentPoints.clear();
            }
        }
    }

    if (!segmentPoints.empty()) {
        func.points.insert(func.points.end(), segmentPoints.begin(), segmentPoints.end());
    }
}

void MultiFunctionPlotter::updateAllFunctions() {
    for (size_t i = 0; i < functions.size(); i++) {
        updateFunction(i);
    }
}

void MultiFunctionPlotter::draw() {
    for (size_t i = 0; i < functions.size(); i++) {
        auto& func = functions[i];
        if (!func.enabled || func.points.empty()) continue;

        MathExpressionParser parser;
        parser.setExpression(func.expression);

        glColor3f(func.color.x, func.color.y, func.color.z);

        if (parser.isCircleEquation()) {
            glBegin(GL_LINE_LOOP);
            for (const auto& point : func.points) {
                glVertex2f(point.x, point.y);
            }
            glEnd();
        } else {
            if (func.points.size() > 1) {
                glBegin(GL_LINE_STRIP);
                for (const auto& point : func.points) {
                    glVertex2f(point.x, point.y);
                }
                glEnd();
            }
        }
    }
}

void MultiFunctionPlotter::clear() {
    functions.clear();
    nextColorIndex = 0;
}

void MultiFunctionPlotter::setRange(float min, float max) {
    xMin = min;
    xMax = max;
    updateAllFunctions();
}

vector<FunctionData>& MultiFunctionPlotter::getFunctions() { return functions; }
float MultiFunctionPlotter::getXMin() const { return xMin; }
float MultiFunctionPlotter::getXMax() const { return xMax; }

void MultiFunctionPlotter::getRange(float& min, float& max) const {
    min = xMin;
    max = xMax;
}