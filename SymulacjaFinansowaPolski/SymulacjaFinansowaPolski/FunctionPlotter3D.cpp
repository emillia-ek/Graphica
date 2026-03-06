#include "FunctionPlotter3D.h"
#include "MathExpressionParser.h"
#include <GLFW/glfw3.h>
#include <cmath>

using namespace std;

FunctionPlotter3D::FunctionPlotter3D() : xMin(-5.0f), xMax(5.0f), yMin(-5.0f), yMax(5.0f), resolution(60) {}

void FunctionPlotter3D::addFunction(const string& expr) {
    FunctionData fd(expr, ImVec4(0.2f, 0.8f, 1.0f, 1.0f));
    functions.push_back(fd);
}

void FunctionPlotter3D::editFunction(int index, const string& newExpr) {
    if (index >= 0 && index < functions.size()) {
        functions[index].expression = newExpr;
        functions[index].enabled = true;
    }
}

void FunctionPlotter3D::removeFunction(int index) {
    if (index >= 0 && index < functions.size()) {
        functions.erase(functions.begin() + index);
    }
}

void FunctionPlotter3D::clear() {
    functions.clear();
}

vector<FunctionData>& FunctionPlotter3D::getFunctions() {
    return functions;
}

void FunctionPlotter3D::setRange(float xMin, float xMax, float yMin, float yMax) {
    this->xMin = xMin;
    this->xMax = xMax;
    this->yMin = yMin;
    this->yMax = yMax;
}

void FunctionPlotter3D::setResolution(int res) {
    this->resolution = res;
    if (this->resolution < 2) this->resolution = 2;
}

void FunctionPlotter3D::draw() {
    MathExpressionParser parser;

    // Rysowanie długich osi 3D
    float axisLen = 10000.0f;
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    // X
    glColor3f(1.0f, 0.3f, 0.3f); glVertex3f(-axisLen, 0.0f, 0.0f); glVertex3f(axisLen, 0.0f, 0.0f);
    // Y
    glColor3f(0.3f, 1.0f, 0.3f); glVertex3f(0.0f, -axisLen, 0.0f); glVertex3f(0.0f, axisLen, 0.0f);
    // Z
    glColor3f(0.3f, 0.5f, 1.0f); glVertex3f(0.0f, 0.0f, -axisLen); glVertex3f(0.0f, 0.0f, axisLen);
    glEnd();
    glLineWidth(1.0f);

    float dx = (xMax - xMin) / resolution;
    float dy = (yMax - yMin) / resolution;

    for (const auto& func : functions) {
        if (!func.enabled) continue;

        parser.setExpression(func.expression);
        if (parser.getErrorMessage().length() > 0 || !parser.is3DFunction()) {
            continue;
        }

        auto type = parser.getType();
        
        // Funkcja będzie w 1 jednolitym kolorze
        glColor3f(func.color.x, func.color.y, func.color.z);

        if (type == SPHERE_3D) {
            float r = parser.getSphereRadius();
            int res = resolution;
            
            // Równoleżniki
            for (int i = 0; i <= res; ++i) {
                float phi = M_PI * float(i) / res;
                glBegin(GL_LINE_LOOP);
                for (int j = 0; j < res; ++j) {
                    float theta = 2.0f * M_PI * float(j) / res;
                    glVertex3f(r * sin(phi) * cos(theta), r * sin(phi) * sin(theta), r * cos(phi));
                }
                glEnd();
            }
            // Południki
            for (int j = 0; j <= res; ++j) {
                float theta = 2.0f * M_PI * float(j) / res;
                glBegin(GL_LINE_STRIP);
                for (int i = 0; i <= res; ++i) {
                    float phi = M_PI * float(i) / res;
                    glVertex3f(r * sin(phi) * cos(theta), r * sin(phi) * sin(theta), r * cos(phi));
                }
                glEnd();
            }
            continue;
        }

        if (type == CONE_3D) {
            float h = 10.0f; // Max z
            int res = resolution;
            
            // Poziome okręgi
            for (int i = 0; i <= res; ++i) {
                float z = -h + 2.0f * h * float(i) / res;
                float r = fabs(z);
                glBegin(GL_LINE_LOOP);
                for (int j = 0; j < res; ++j) {
                    float theta = 2.0f * M_PI * float(j) / res;
                    glVertex3f(r * cos(theta), r * sin(theta), z);
                }
                glEnd();
            }
            // Pionowe linie - generatrysy
            for (int j = 0; j <= res; ++j) {
                float theta = 2.0f * M_PI * float(j) / res;
                glBegin(GL_LINE_STRIP);
                for (int i = 0; i <= res; ++i) {
                    float z = -h + 2.0f * h * float(i) / res;
                    float r = fabs(z);
                    glVertex3f(r * cos(theta), r * sin(theta), z);
                }
                glEnd();
            }
            continue;
        }

        // Standardowa siatka dla z = f(x,y)
        // Linie wzdłuż osi X
        for (int i = 0; i <= resolution; ++i) {
            float y = yMin + i * dy;
            glBegin(GL_LINE_STRIP);
            for (int j = 0; j <= resolution; ++j) {
                float x = xMin + j * dx;
                
                float z = parser.evaluate(x, y);
                if (!std::isnan(z)) {
                    glVertex3f(x, y, z);
                } else {
                    glEnd();
                    glBegin(GL_LINE_STRIP);
                }
            }
            glEnd();
        }

        // Linie wzdłuż osi Y
        for (int j = 0; j <= resolution; ++j) {
            float x = xMin + j * dx;
            glBegin(GL_LINE_STRIP);
            for (int i = 0; i <= resolution; ++i) {
                float y = yMin + i * dy;
                
                float z = parser.evaluate(x, y);
                if (!std::isnan(z)) {
                    glVertex3f(x, y, z);
                } else {
                    glEnd();
                    glBegin(GL_LINE_STRIP);
                }
            }
            glEnd();
        }
    }
}
