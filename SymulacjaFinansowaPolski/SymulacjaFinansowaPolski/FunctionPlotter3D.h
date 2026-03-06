#ifndef FUNCTIONPLOTTER3D_H
#define FUNCTIONPLOTTER3D_H

#include "FunctionData.h"
#include <vector>

class FunctionPlotter3D {
private:
    std::vector<FunctionData> functions;
    float xMin, xMax, yMin, yMax;
    int resolution;

public:
    FunctionPlotter3D();

    void addFunction(const std::string& expr);
    void editFunction(int index, const std::string& newExpr);
    void removeFunction(int index);
    void clear();

    std::vector<FunctionData>& getFunctions();

    void setRange(float xMin, float xMax, float yMin, float yMax);
    void setResolution(int res);
    
    void draw();
};

#endif
