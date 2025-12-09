#include "FunctionData.h"

FunctionData::FunctionData(const std::string& expr, const ImVec4& col)
    : expression(expr), color(col), enabled(true), editing(false), editBuffer(expr) {}

void FunctionData::startEditing() {
    editing = true;
    editBuffer = expression;
}

void FunctionData::applyEdit() {
    if (!editBuffer.empty()) {
        expression = editBuffer;
    }
    editing = false;
}

void FunctionData::cancelEdit() {
    editing = false;
}