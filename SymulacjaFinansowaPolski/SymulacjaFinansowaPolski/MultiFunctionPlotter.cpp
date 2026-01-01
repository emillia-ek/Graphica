#include "MultiFunctionPlotter.h"
#include "MathExpressionParser.h"
#include <GLFW/glfw3.h>
#include <cmath>
#include <vector>

using namespace std;

MultiFunctionPlotter::MultiFunctionPlotter() : xMin(-10.0f), xMax(10.0f), resolution(2000), nextColorIndex(0) {
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

void MultiFunctionPlotter::updateFunction(int index) {
    if (index < 0 || index >= (int)functions.size()) return;

    auto& func = functions[index];
    func.points.clear();

    MathExpressionParser parser;
    parser.setExpression(func.expression);
    
    if (!parser.getErrorMessage().empty() && parser.getType() == UNKNOWN) return;

    // 1. Linie pionowe
    if (parser.getType() == VERTICAL_LINE) {
        float xValue = parser.getVerticalLineX();
        if (!isnan(xValue)) {
            func.points.emplace_back(xValue, -1000.0f);
            func.points.emplace_back(xValue, 1000.0f);
        }
        return;
    }

    // 2. Linie poziome
    if (parser.getType() == HORIZONTAL_LINE) {
        float yValue = parser.getHorizontalLineY();
        if (!isnan(yValue)) {
            func.points.emplace_back(xMin, yValue);
            func.points.emplace_back(xMax, yValue);
        }
        return;
    }

    // 3. Okręgi
    if (parser.isCircleEquation()) {
        float cx, cy, r;
        parser.getCircleParams(cx, cy, r);
        int circlePoints = 360;
        for (int i = 0; i <= circlePoints; i++) {
            float angle = 2.0f * (float)M_PI * i / (float)circlePoints;
            func.points.emplace_back(cx + r * cos(angle), cy + r * sin(angle));
        }
        return;
    }

    //
    float step = (xMax - xMin) / (float)resolution;

    for (int i = 0; i <= resolution; ++i) {
            float x = xMin + i * step;
            float y = parser.evaluate(x);

        if (!isnan(y) && !isinf(y)) {
            if (!func.points.empty() && !isnan(func.points.back().y)) {
                if (fabs(y - func.points.back().y) > 1.9f) {
                    func.points.emplace_back(NAN, NAN);
                }
            }
            func.points.emplace_back(x, y);
        } else {
            if (!func.points.empty() && !isnan(func.points.back().x)) {
                func.points.emplace_back(NAN, NAN);
            }
        }
    }
}


void MultiFunctionPlotter::draw() {
    for (auto& func : functions) {
        if (!func.enabled || func.points.empty()) continue;
        glColor3f(func.color.x, func.color.y, func.color.z);
        glLineWidth(2.0f);

        glBegin(GL_LINE_STRIP);
        for (const auto& pt : func.points) {
            if (isnan(pt.x) || isnan(pt.y)) {
                glEnd();
                glBegin(GL_LINE_STRIP);
            } else {
                glVertex2f(pt.x, pt.y);
            }
        }
        glEnd();
    }
    glLineWidth(1.0f);
}

void MultiFunctionPlotter::setRange(float min, float max) {
    xMin = min;
    xMax = max;
    updateAllFunctions(); // wymusza przeliczenie punktów dla nowego widoku
}

void MultiFunctionPlotter::updateAllFunctions() {
    for (size_t i = 0; i < functions.size(); i++) {
        updateFunction((int)i);
    }
}

void MultiFunctionPlotter::addFunction(const string& equation) {
    ImVec4 color = colorPalette[nextColorIndex % colorPalette.size()];
    functions.emplace_back(equation, color);
    nextColorIndex++;
    updateFunction((int)functions.size() - 1);
}

void MultiFunctionPlotter::editFunction(int index, const string& newEquation) {
    if (index >= 0 && index < (int)functions.size()) {
        functions[index].expression = newEquation;
        updateFunction(index);
    }
}

void MultiFunctionPlotter::removeFunction(int index) {
    if (index >= 0 && index < (int)functions.size()) {
        functions.erase(functions.begin() + index);
    }
}

void MultiFunctionPlotter::clear() {
    functions.clear();
    nextColorIndex = 0;
}

vector<FunctionData>& MultiFunctionPlotter::getFunctions() { return functions; }
float MultiFunctionPlotter::getXMin() const { return xMin; }
float MultiFunctionPlotter::getXMax() const { return xMax; }
void MultiFunctionPlotter::getRange(float& min, float& max) const { min = xMin; max = xMax; }
