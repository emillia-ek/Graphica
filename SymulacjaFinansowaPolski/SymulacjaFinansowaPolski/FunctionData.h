#ifndef FUNCTIONDATA_H
#define FUNCTIONDATA_H

#include <string>
#include <vector>
#include "imgui.h"
#include "Point.h"

struct FunctionData {
    std::string expression;
    std::vector<Point> points;
    ImVec4 color;
    bool enabled;
    bool editing;
    std::string editBuffer;

    FunctionData(const std::string& expr, const ImVec4& col);
    void startEditing();
    void applyEdit();
    void cancelEdit();
};

#endif