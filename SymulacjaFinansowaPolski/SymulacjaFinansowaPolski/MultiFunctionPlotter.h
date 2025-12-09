#ifndef MULTIFUNCTIONPLOTTER_H
#define MULTIFUNCTIONPLOTTER_H

#include <vector>
#include <string>
#include "FunctionData.h"
#include "imgui.h"

class MultiFunctionPlotter {
private:
    std::vector<FunctionData> functions;
    float xMin, xMax;
    int resolution;
    std::vector<ImVec4> colorPalette;
    int nextColorIndex;

public:
    MultiFunctionPlotter();
    void addFunction(const std::string& equation);
    void editFunction(int index, const std::string& newEquation);
    void removeFunction(int index);
    void updateFunction(int index);
    void updateAllFunctions();
    void draw();
    void clear();
    void setRange(float min, float max);
    std::vector<FunctionData>& getFunctions();
    float getXMin() const;
    float getXMax() const;
    void getRange(float& min, float& max) const;
};

#endif