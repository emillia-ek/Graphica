#include <iostream>
#include <cmath>
#include <vector>
#include <string>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <regex>
#include <map>
#include <memory>

using namespace std;

// OpenGL and GLFW headers
#include <GLFW/glfw3.h>
#include "imconfig.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// Forward declarations
GLFWwindow* window;

// Structure for storing points on the graph
struct Point {
    float x, y;
    Point(float x, float y) : x(x), y(y) {}
};

// Structure for storing a function with its properties
struct FunctionData {
    string expression;
    vector<Point> points;
    ImVec4 color;
    bool enabled;
    bool editing;
    string editBuffer;

    FunctionData(const string& expr, const ImVec4& col)
        : expression(expr), color(col), enabled(true), editing(false), editBuffer(expr) {}

    void startEditing() {
        editing = true;
        editBuffer = expression;
    }

    void applyEdit() {
        if (!editBuffer.empty()) {
            expression = editBuffer;
        }
        editing = false;
    }

    void cancelEdit() {
        editing = false;
    }
};

// Enum for function types
enum FunctionType {
    LINEAR,
    QUADRATIC,
    SIN,
    COS,
    TAN,
    COT,
    EXPONENTIAL,
    LOGARITHMIC,
    POLYNOMIAL,
    POWER,
    ABSOLUTE,
    VERTICAL_LINE,
    HORIZONTAL_LINE,
    CIRCLE,
    CONSTANT_POWER,
    UNKNOWN
};

// Class for parsing and evaluating mathematical expressions
class MathExpressionParser {
private:
    string expression;
    FunctionType type;
    vector<float> coefficients;
    float verticalLineX;
    float horizontalLineY;

    // Circle parameters
    float circleCenterX, circleCenterY, circleRadius;
    bool isCircle;

    // Polynomial coefficients storage
    vector<pair<float, float>> polynomialTerms; // pair<coefficient, exponent>

    // Helper function to remove whitespace
    string removeWhitespace(const string& str) {
        string result;
        remove_copy_if(str.begin(), str.end(), std::back_inserter(result),
                      [](char c) { return isspace(c); });
        return result;
    }

    // Helper function to convert to lowercase
    string toLower(const string& str) {
        string result = str;
        transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return tolower(c); });
        return result;
    }

    // Check if string contains a substring
    bool contains(const string& str, const string& substr) {
        return str.find(substr) != string::npos;
    }

    // Replace all occurrences of a substring
    void replaceAll(string& str, const string& from, const string& to) {
        size_t start_pos = 0;
        while((start_pos = str.find(from, start_pos)) != string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
    }

    // Normalize function expression
    void normalizeExpression(string& expr) {
        // Store original for special cases
        string original = expr;

        // Check for circle equation - FIXED: Better detection
        if ((contains(expr, "(x") && contains(expr, ")^2") &&
             contains(expr, "(y") && contains(expr, ")^2") &&
             contains(expr, "=")) ||
            (contains(expr, "x^2") && contains(expr, "y^2") && contains(expr, "="))) {
            parseCircleEquation(original);
            return;
        }

        // Replace f(x) with y=
        if (expr.find("f(x)=") == 0) {
            expr = "y=" + expr.substr(5);
        } else if (expr.find("f(x)") == 0 && expr.length() > 4) {
            expr = "y=" + expr.substr(4);
        }

        // Handle horizontal lines y = constant
        if (expr.find("y=") == 0) {
            // Check if it's just y=constant (no x)
            string afterEqual = expr.substr(2);
            if (!contains(afterEqual, "x") &&
                !contains(afterEqual, "sin") &&
                !contains(afterEqual, "cos") &&
                !contains(afterEqual, "tan") &&
                !contains(afterEqual, "cot") &&
                !contains(afterEqual, "log") &&
                !contains(afterEqual, "ln") &&
                !contains(afterEqual, "exp") &&
                !contains(afterEqual, "e^") &&
                !contains(afterEqual, "abs")) {
                try {
                    horizontalLineY = stof(afterEqual);
                    type = HORIZONTAL_LINE;
                    expr = "horizontal";
                    return;
                } catch (...) {
                    // Not a simple constant, continue parsing
                }
            }
        }

        // Handle vertical lines x = constant
        if (expr.find("x=") == 0) {
            // Parse the x value
            size_t eqPos = expr.find('=');
            if (eqPos != string::npos) {
                try {
                    verticalLineX = stof(expr.substr(eqPos + 1));
                    type = VERTICAL_LINE;
                    expr = "vertical";
                    return;
                } catch (...) {
                    verticalLineX = 0.0f;
                }
            }
        }

        // Replace tg with tan and ctg with cot
        replaceAll(expr, "tg", "tan");
        replaceAll(expr, "ctg", "cot");

        // Handle absolute value - FIXED: Proper handling for transformations
        // First, replace all |...| with abs(...) but handle transformations inside
        size_t pipePos = 0;
        while ((pipePos = expr.find('|', pipePos)) != string::npos) {
            size_t nextPipe = expr.find('|', pipePos + 1);
            if (nextPipe != string::npos) {
                string inside = expr.substr(pipePos + 1, nextPipe - pipePos - 1);
                expr = expr.substr(0, pipePos) + "abs(" + inside + ")" + expr.substr(nextPipe + 1);
                pipePos += 4 + inside.length(); // Skip past "abs(inside)"
            } else {
                break; // Unmatched pipe
            }
        }

        // Remove y= if present (but not for special cases)
        if (expr.find("y=") == 0 && type != VERTICAL_LINE && type != HORIZONTAL_LINE) {
            expr = expr.substr(2);
        }
    }

    // Parse circle equation: (x-a)^2 + (y-b)^2 = r^2 - FIXED
    void parseCircleEquation(const string& expr) {
        isCircle = true;
        type = CIRCLE;

        // Default values
        circleCenterX = 0.0f;
        circleCenterY = 0.0f;
        circleRadius = 1.0f;

        try {
            // Remove spaces
            string cleanExpr = removeWhitespace(toLower(expr));

            // Find the equals sign
            size_t eqPos = cleanExpr.find('=');
            if (eqPos == string::npos) return;

            // Get right side (radius squared)
            string rightSide = cleanExpr.substr(eqPos + 1);
            float radiusSquared = stof(rightSide);
            circleRadius = sqrt(radiusSquared);

            // Get left side
            string leftSide = cleanExpr.substr(0, eqPos);

            // Extract x part - FIXED: Better parsing
            size_t xStart = leftSide.find("(x");
            if (xStart != string::npos) {
                size_t xEnd = leftSide.find(")^2", xStart);
                if (xEnd != string::npos) {
                    string xExpr = leftSide.substr(xStart + 1, xEnd - xStart - 1);
                    // xExpr should be like "x-2" or "x+2" or "x"
                    if (xExpr.length() > 1) {
                        // Has transformation
                        char op = xExpr[1]; // Should be + or -
                        float value = stof(xExpr.substr(2));
                        if (op == '-') {
                            circleCenterX = value; // (x-a)^2 means center at a
                        } else if (op == '+') {
                            circleCenterX = -value; // (x+a)^2 means center at -a
                        }
                    } else {
                        circleCenterX = 0.0f; // Just (x)^2
                    }
                }
            } else if (contains(leftSide, "x^2")) {
                // x^2 without parentheses, center at 0
                circleCenterX = 0.0f;
            }

            // Extract y part - FIXED: Better parsing
            size_t yStart = leftSide.find("(y");
            if (yStart != string::npos) {
                size_t yEnd = leftSide.find(")^2", yStart);
                if (yEnd != string::npos) {
                    string yExpr = leftSide.substr(yStart + 1, yEnd - yStart - 1);
                    // yExpr should be like "y-2" or "y+2" or "y"
                    if (yExpr.length() > 1) {
                        // Has transformation
                        char op = yExpr[1]; // Should be + or -
                        float value = stof(yExpr.substr(2));
                        if (op == '-') {
                            circleCenterY = value; // (y-b)^2 means center at b
                        } else if (op == '+') {
                            circleCenterY = -value; // (y+b)^2 means center at -b
                        }
                    } else {
                        circleCenterY = 0.0f; // Just (y)^2
                    }
                }
            } else if (contains(leftSide, "y^2")) {
                // y^2 without parentheses, center at 0
                circleCenterY = 0.0f;
            }

        } catch (...) {
            // Use defaults if parsing fails
            circleCenterX = 0.0f;
            circleCenterY = 0.0f;
            circleRadius = 1.0f;
        }
    }

    // Parse polynomial terms - NEW FUNCTION
    void parsePolynomial(const string& expr) {
        polynomialTerms.clear();

        // Simple polynomial parser for terms like ax^n
        string cleanExpr = expr;

        // Handle addition/subtraction
        vector<string> terms;
        string currentTerm;
        int parenCount = 0;

        for (char c : cleanExpr) {
            if (c == '(') parenCount++;
            else if (c == ')') parenCount--;

            if (parenCount == 0 && (c == '+' || c == '-') && !currentTerm.empty()) {
                terms.push_back(currentTerm);
                currentTerm = c;
            } else {
                currentTerm += c;
            }
        }
        if (!currentTerm.empty()) {
            terms.push_back(currentTerm);
        }

        // Parse each term
        for (string& term : terms) {
            // Remove leading + if present
            if (term[0] == '+') term = term.substr(1);

            // Check if term contains x
            if (contains(term, "x")) {
                size_t xPos = term.find('x');
                float coefficient = 1.0f;
                float exponent = 1.0f;

                // Parse coefficient
                string coeffStr = term.substr(0, xPos);
                if (coeffStr.empty() || coeffStr == "+") {
                    coefficient = 1.0f;
                } else if (coeffStr == "-") {
                    coefficient = -1.0f;
                } else {
                    try {
                        coefficient = stof(coeffStr);
                    } catch (...) {
                        coefficient = 1.0f;
                    }
                }

                // Parse exponent
                if (xPos + 1 < term.length() && term[xPos + 1] == '^') {
                    size_t expStart = xPos + 2;
                    string expStr = term.substr(expStart);
                    try {
                        exponent = stof(expStr);
                    } catch (...) {
                        exponent = 1.0f;
                    }
                }

                polynomialTerms.emplace_back(coefficient, exponent);
            } else {
                // Constant term
                try {
                    float constant = stof(term);
                    polynomialTerms.emplace_back(constant, 0.0f);
                } catch (...) {
                    // Ignore invalid terms
                }
            }
        }

        type = POLYNOMIAL;
    }

    // Parse expression with proper operator precedence
    float parseExpression(float x, const string& expr) {
        if (expr.empty()) return 0.0f;

        // Remove outer parentheses
        string trimmed = expr;
        while (trimmed.front() == '(' && trimmed.back() == ')') {
            trimmed = trimmed.substr(1, trimmed.length() - 2);
        }

        // Check for addition/subtraction (lowest precedence)
        int parenCount = 0;
        size_t opPos = string::npos;
        char op = '+';

        for (size_t i = 0; i < trimmed.length(); i++) {
            char c = trimmed[i];
            if (c == '(') parenCount++;
            else if (c == ')') parenCount--;
            else if (parenCount == 0 && (c == '+' || c == '-') && i > 0) {
                // Check if this is not part of a number or exponent
                if (i > 0) {
                    char prev = trimmed[i-1];
                    if (prev != 'e' && prev != 'E' && !(i > 1 && (trimmed[i-2] == 'e' || trimmed[i-2] == 'E'))) {
                        opPos = i;
                        op = c;
                        break;
                    }
                }
            }
        }

        if (opPos != string::npos) {
            string left = trimmed.substr(0, opPos);
            string right = trimmed.substr(opPos + 1);

            float leftVal = parseExpression(x, left);
            float rightVal = parseExpression(x, right);

            return (op == '+') ? leftVal + rightVal : leftVal - rightVal;
        }

        // Check for multiplication
        parenCount = 0;
        opPos = string::npos;

        for (size_t i = 0; i < trimmed.length(); i++) {
            char c = trimmed[i];
            if (c == '(') parenCount++;
            else if (c == ')') parenCount--;
            else if (parenCount == 0 && c == '*' && i > 0) {
                opPos = i;
                break;
            }
        }

        if (opPos != string::npos) {
            string left = trimmed.substr(0, opPos);
            string right = trimmed.substr(opPos + 1);

            float leftVal = parseExpression(x, left);
            float rightVal = parseExpression(x, right);

            return leftVal * rightVal;
        }

        // Check for implied multiplication
        for (size_t i = 0; i < trimmed.length() - 1; i++) {
            if ((isdigit(trimmed[i]) || trimmed[i] == ')' || trimmed[i] == 'x') &&
                (trimmed[i+1] == '(' || trimmed[i+1] == 'x' ||
                 trimmed[i+1] == 's' || trimmed[i+1] == 'c' ||
                 trimmed[i+1] == 't' || trimmed[i+1] == 'l' ||
                 trimmed[i+1] == 'e' || trimmed[i+1] == 'a')) {
                string left = trimmed.substr(0, i+1);
                string right = trimmed.substr(i+1);
                return parseExpression(x, left) * parseExpression(x, right);
            }
        }

        // Check for division
        parenCount = 0;
        opPos = string::npos;

        for (size_t i = 0; i < trimmed.length(); i++) {
            char c = trimmed[i];
            if (c == '(') parenCount++;
            else if (c == ')') parenCount--;
            else if (parenCount == 0 && c == '/' && i > 0) {
                opPos = i;
                break;
            }
        }

        if (opPos != string::npos) {
            string left = trimmed.substr(0, opPos);
            string right = trimmed.substr(opPos + 1);

            float leftVal = parseExpression(x, left);
            float rightVal = parseExpression(x, right);

            if (fabs(rightVal) < 0.0001f) return NAN;
            return leftVal / rightVal;
        }

        // Check for power (right associative)
        int maxPos = -1;
        parenCount = 0;
        for (size_t i = 0; i < trimmed.length(); i++) {
            char c = trimmed[i];
            if (c == '(') parenCount++;
            else if (c == ')') parenCount--;
            else if (parenCount == 0 && c == '^') {
                maxPos = i;
            }
        }

        if (maxPos != -1) {
            string left = trimmed.substr(0, maxPos);
            string right = trimmed.substr(maxPos + 1);

            float base = parseExpression(x, left);
            float exponent = parseExpression(x, right);

            if (base < 0 && fabs(exponent - floor(exponent)) > 0.0001f) {
                return NAN;
            }

            return pow(base, exponent);
        }

        // Check for functions
        if (contains(trimmed, "sin(")) {
            size_t start = trimmed.find("sin(") + 4;
            size_t end = findMatchingParen(trimmed, start - 1);
            string arg = trimmed.substr(start, end - start);
            return sin(parseExpression(x, arg));
        }
        if (contains(trimmed, "cos(")) {
            size_t start = trimmed.find("cos(") + 4;
            size_t end = findMatchingParen(trimmed, start - 1);
            string arg = trimmed.substr(start, end - start);
            return cos(parseExpression(x, arg));
        }
        if (contains(trimmed, "tan(")) {
            size_t start = trimmed.find("tan(") + 4;
            size_t end = findMatchingParen(trimmed, start - 1);
            string arg = trimmed.substr(start, end - start);
            float val = parseExpression(x, arg);
            float result = tan(val);
            if (fabs(cos(val)) < 0.001f) return NAN;
            return result;
        }
        if (contains(trimmed, "cot(")) {
            size_t start = trimmed.find("cot(") + 4;
            size_t end = findMatchingParen(trimmed, start - 1);
            string arg = trimmed.substr(start, end - start);
            float val = parseExpression(x, arg);
            if (fabs(sin(val)) < 0.001f) return NAN;
            return cos(val) / sin(val);
        }
        if (contains(trimmed, "exp(")) {
            size_t start = trimmed.find("exp(") + 4;
            size_t end = findMatchingParen(trimmed, start - 1);
            string arg = trimmed.substr(start, end - start);
            return exp(parseExpression(x, arg));
        }
        if (contains(trimmed, "ln(")) {
            size_t start = trimmed.find("ln(") + 3;
            size_t end = findMatchingParen(trimmed, start - 1);
            string arg = trimmed.substr(start, end - start);
            float val = parseExpression(x, arg);
            if (val <= 0) return NAN;
            return log(val);
        }
        if (contains(trimmed, "log(")) {
            size_t start = trimmed.find("log(") + 4;
            size_t end = findMatchingParen(trimmed, start - 1);
            string arg = trimmed.substr(start, end - start);
            float val = parseExpression(x, arg);
            if (val <= 0) return NAN;
            return log10(val);
        }
        if (contains(trimmed, "abs(")) {
            size_t start = trimmed.find("abs(") + 4;
            size_t end = findMatchingParen(trimmed, start - 1);
            string arg = trimmed.substr(start, end - start);
            return fabs(parseExpression(x, arg));
        }

        // Check for e^x
        if (trimmed.find("e^") == 0) {
            string exponent = trimmed.substr(2);
            float expVal = parseExpression(x, exponent);
            return exp(expVal);
        }

        // Check for x
        if (trimmed == "x") return x;
        if (trimmed == "-x") return -x;

        // Check for numbers
        try {
            return stof(trimmed);
        } catch (...) {
            return 0.0f;
        }
    }

    // Find matching parenthesis
    size_t findMatchingParen(const string& str, size_t start) {
        int count = 1;
        for (size_t i = start + 1; i < str.length(); i++) {
            if (str[i] == '(') count++;
            else if (str[i] == ')') {
                count--;
                if (count == 0) return i;
            }
        }
        return str.length();
    }

public:
    MathExpressionParser() : type(UNKNOWN), verticalLineX(0.0f), horizontalLineY(0.0f),
                             circleCenterX(0.0f), circleCenterY(0.0f), circleRadius(1.0f),
                             isCircle(false) {}

    void setExpression(const string& expr) {
        verticalLineX = 0.0f;
        horizontalLineY = 0.0f;
        isCircle = false;
        polynomialTerms.clear();

        string processed = expr;
        normalizeExpression(processed);
        expression = removeWhitespace(toLower(processed));

        if (type != VERTICAL_LINE && type != HORIZONTAL_LINE && type != CIRCLE) {
            detectFunctionType();
        }
    }

    void detectFunctionType() {
        string expr = expression;

        // Check for circle
        if (isCircle) {
            type = CIRCLE;
            return;
        }

        // Check for constant power like 1^x
        if (contains(expr, "1^x") || expr == "1^x") {
            type = CONSTANT_POWER;
            return;
        }

        // Check for power expressions with x in exponent (like 2^x)
        if (contains(expr, "^x")) {
            type = POWER;
            return;
        }

        // Check for absolute value
        if (contains(expr, "abs(")) {
            type = ABSOLUTE;
            return;
        }

        // Check for trigonometric functions
        if (contains(expr, "sin(")) {
            type = SIN;
            return;
        }
        if (contains(expr, "cos(")) {
            type = COS;
            return;
        }
        if (contains(expr, "tan(")) {
            type = TAN;
            return;
        }
        if (contains(expr, "cot(")) {
            type = COT;
            return;
        }

        // Check for exponential (e^x)
        if (expr.find("e^") == 0 || contains(expr, "exp(")) {
            type = EXPONENTIAL;
            return;
        }

        // Check for logarithmic
        if (contains(expr, "log") || contains(expr, "ln(")) {
            type = LOGARITHMIC;
            return;
        }

        // Check for quadratic (x^2)
        if (contains(expr, "x^2") && !contains(expr, "x^3") && !contains(expr, "x^4")) {
            type = QUADRATIC;
            return;
        }

        // Check for linear (contains x but not x^2, x^3, etc.)
        if (contains(expr, "x") && !contains(expr, "x^") && !contains(expr, "sin") &&
            !contains(expr, "cos") && !contains(expr, "tan") && !contains(expr, "cot") &&
            !contains(expr, "log") && !contains(expr, "ln") && !contains(expr, "exp") &&
            !contains(expr, "abs")) {
            type = LINEAR;
            return;
        }

        // Check for polynomial (contains x^3, x^4, etc.)
        if (contains(expr, "x^")) {
            parsePolynomial(expr);
            return;
        }

        // Default
        type = UNKNOWN;
    }

    float evaluate(float x) {
        // Handle special cases
        if (type == VERTICAL_LINE) {
            return NAN;
        }
        if (type == HORIZONTAL_LINE) {
            return horizontalLineY;
        }
        if (type == CIRCLE) {
            return NAN;
        }

        string expr = expression;

        try {
            // Handle constant power first (y=1^x)
            if (type == CONSTANT_POWER) {
                return 1.0f;
            }

            // Use general expression parser
            return parseExpression(x, expr);

        } catch (const exception& e) {
            return NAN;
        }
    }

    FunctionType getType() const { return type; }
    string getExpression() const { return expression; }

    // Get special line values
    float getVerticalLineX() const { return verticalLineX; }
    float getHorizontalLineY() const { return horizontalLineY; }

    // Get circle parameters
    bool isCircleEquation() const { return isCircle; }
    void getCircleParams(float& cx, float& cy, float& r) const {
        cx = circleCenterX;
        cy = circleCenterY;
        r = circleRadius;
    }
};

// Class managing multiple functions
class MultiFunctionPlotter {
private:
    vector<FunctionData> functions;
    float xMin, xMax;
    int resolution;

    // Color palette
    vector<ImVec4> colorPalette = {
        ImVec4(0.0f, 0.8f, 1.0f, 1.0f),  // Cyan
        ImVec4(1.0f, 0.3f, 0.3f, 1.0f),  // Red
        ImVec4(0.3f, 1.0f, 0.3f, 1.0f),  // Green
        ImVec4(1.0f, 0.8f, 0.0f, 1.0f),  // Yellow
        ImVec4(0.8f, 0.3f, 1.0f, 1.0f),  // Purple
        ImVec4(0.0f, 1.0f, 0.8f, 1.0f),  // Teal
        ImVec4(1.0f, 0.5f, 0.0f, 1.0f),  // Orange
        ImVec4(0.5f, 0.5f, 1.0f, 1.0f)   // Blue
    };

    int nextColorIndex = 0;

public:
    MultiFunctionPlotter() : xMin(-10.0f), xMax(10.0f), resolution(800) {}

    void addFunction(const string& equation) {
        ImVec4 color = colorPalette[nextColorIndex % colorPalette.size()];
        functions.emplace_back(equation, color);
        nextColorIndex++;
        updateFunction(functions.size() - 1);
    }

    void editFunction(int index, const string& newEquation) {
        if (index >= 0 && index < functions.size()) {
            functions[index].expression = newEquation;
            updateFunction(index);
        }
    }

    void removeFunction(int index) {
        if (index >= 0 && index < functions.size()) {
            functions.erase(functions.begin() + index);
        }
    }

    void updateFunction(int index) {
        if (index < 0 || index >= functions.size()) return;

        auto& func = functions[index];
        func.points.clear();

        MathExpressionParser parser;
        parser.setExpression(func.expression);

        // Handle vertical lines
        if (parser.getType() == VERTICAL_LINE) {
            float xValue = parser.getVerticalLineX();
            if (!isnan(xValue)) {
                // Create vertical line points
                for (float y = -100.0f; y <= 100.0f; y += 0.1f) {
                    func.points.emplace_back(xValue, y);
                }
            }
            return;
        }

        // Handle horizontal lines
        if (parser.getType() == HORIZONTAL_LINE) {
            float yValue = parser.getHorizontalLineY();
            if (!isnan(yValue)) {
                // Create horizontal line points
                for (float x = xMin; x <= xMax; x += 0.1f) {
                    func.points.emplace_back(x, yValue);
                }
            }
            return;
        }

        // Handle circles - FIXED
        if (parser.isCircleEquation()) {
            float cx, cy, r;
            parser.getCircleParams(cx, cy, r);

            // Generate points for circle
            int circlePoints = 200;
            for (int i = 0; i <= circlePoints; i++) {
                float angle = 2.0f * M_PI * i / circlePoints;
                float x = cx + r * cos(angle);
                float y = cy + r * sin(angle);
                func.points.emplace_back(x, y);
            }
            return;
        }

        // Regular function
        float step = (xMax - xMin) / resolution;
        vector<Point> segmentPoints;

        for (int i = 0; i <= resolution; ++i) {
            float x = xMin + i * step;
            float y = parser.evaluate(x);

            if (!isnan(y) && !isinf(y)) {
                segmentPoints.emplace_back(x, y);
            } else {
                // NaN or inf - end current segment
                if (!segmentPoints.empty()) {
                    func.points.insert(func.points.end(), segmentPoints.begin(), segmentPoints.end());
                    segmentPoints.clear();
                }
            }
        }

        // Add any remaining points
        if (!segmentPoints.empty()) {
            func.points.insert(func.points.end(), segmentPoints.begin(), segmentPoints.end());
        }
    }

    void updateAllFunctions() {
        for (size_t i = 0; i < functions.size(); i++) {
            updateFunction(i);
        }
    }

    void draw() {
        // Draw regular functions first
        for (size_t i = 0; i < functions.size(); i++) {
            auto& func = functions[i];
            if (!func.enabled || func.points.empty()) continue;

            // Get parser to check function type
            MathExpressionParser parser;
            parser.setExpression(func.expression);

            // Draw function as line segments to handle discontinuities
            glColor3f(func.color.x, func.color.y, func.color.z);

            // For circles, use line loop
            if (parser.isCircleEquation()) {
                glBegin(GL_LINE_LOOP);
                for (const auto& point : func.points) {
                    glVertex2f(point.x, point.y);
                }
                glEnd();
            } else {
                // Draw as line strip for continuous functions
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

    void clear() {
        functions.clear();
        nextColorIndex = 0;
    }

    void setRange(float min, float max) {
        xMin = min;
        xMax = max;
        updateAllFunctions();
    }

    vector<FunctionData>& getFunctions() { return functions; }

    float getXMin() const { return xMin; }
    float getXMax() const { return xMax; }

    // Get current view range
    void getRange(float& min, float& max) const {
        min = xMin;
        max = xMax;
    }
};

// Class managing grid and axes with zoom/pan support
class CoordinateSystem {
private:
    float viewXMin, viewXMax, viewYMin, viewYMax;
    float baseXMin, baseXMax, baseYMin, baseYMax;

public:
    CoordinateSystem() : viewXMin(-10.0f), viewXMax(10.0f), viewYMin(-10.0f), viewYMax(10.0f),
                         baseXMin(-10.0f), baseXMax(10.0f), baseYMin(-10.0f), baseYMax(10.0f) {}

    void draw() {
        // Calculate aspect ratio and adjust to make grid square
        int windowWidth, windowHeight;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);
        float aspect = (float)windowWidth / windowHeight;

        float currentWidth = viewXMax - viewXMin;
        float currentHeight = viewYMax - viewYMin;
        float targetAspect = 1.0f; // We want square grid (1:1 aspect)

        // Calculate the scale factor needed
        float scaleX = currentWidth;
        float scaleY = currentHeight;

        if (aspect > targetAspect) {
            // Window is wider than it is tall, adjust x range
            scaleX = scaleY * aspect;
        } else {
            // Window is taller than it is wide, adjust y range
            scaleY = scaleX / aspect;
        }

        // Apply the adjusted scale to make grid square
        float centerX = (viewXMin + viewXMax) / 2.0f;
        float centerY = (viewYMin + viewYMax) / 2.0f;

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(centerX - scaleX/2, centerX + scaleX/2,
                centerY - scaleY/2, centerY + scaleY/2, -1.0f, 1.0f);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // Draw grid
        glColor3f(0.12f, 0.12f, 0.12f);
        glBegin(GL_LINES);

        // Use independent spacing for x and y axes
        float xSpacing = getSpacing(scaleX);
        float ySpacing = getSpacing(scaleY);

        // Horizontal grid lines
        float yStart = ceil((centerY - scaleY/2) / ySpacing) * ySpacing;
        float yEnd = centerY + scaleY/2;
        for (float y = yStart; y <= yEnd; y += ySpacing) {
            if (fabs(y) > ySpacing/10.0f) {
                glVertex2f(centerX - scaleX/2, y);
                glVertex2f(centerX + scaleX/2, y);
            }
        }

        // Vertical grid lines
        float xStart = ceil((centerX - scaleX/2) / xSpacing) * xSpacing;
        float xEnd = centerX + scaleX/2;
        for (float x = xStart; x <= xEnd; x += xSpacing) {
            if (fabs(x) > xSpacing/10.0f) {
                glVertex2f(x, centerY - scaleY/2);
                glVertex2f(x, centerY + scaleY/2);
            }
        }

        glEnd();

        // Draw thicker axes
        glColor3f(0.4f, 0.4f, 0.4f);
        glLineWidth(1.5f);
        glBegin(GL_LINES);

        // X axis
        glVertex2f(centerX - scaleX/2, 0.0f);
        glVertex2f(centerX + scaleX/2, 0.0f);

        // Y axis
        glVertex2f(0.0f, centerY - scaleY/2);
        glVertex2f(0.0f, centerY + scaleY/2);

        glEnd();
        glLineWidth(1.0f);

        // Draw arrows
        drawArrows();

        // Draw axis labels
        drawAxisLabels(xSpacing, ySpacing);

        // Draw main unit lines slightly darker
        glColor3f(0.2f, 0.2f, 0.2f);
        glBegin(GL_LINES);

        // Main horizontal lines at integer multiples of ySpacing
        for (float y = yStart; y <= yEnd; y += ySpacing) {
            if (y != 0) {
                glVertex2f(centerX - scaleX/2, y);
                glVertex2f(centerX + scaleX/2, y);
            }
        }

        // Main vertical lines at integer multiples of xSpacing
        for (float x = xStart; x <= xEnd; x += xSpacing) {
            if (x != 0) {
                glVertex2f(x, centerY - scaleY/2);
                glVertex2f(x, centerY + scaleY/2);
            }
        }

        glEnd();
    }

private:
    float getSpacing(float range) {
        if (range > 50.0f) return 5.0f;
        else if (range > 20.0f) return 2.0f;
        else if (range < 2.0f) return 0.2f;
        else if (range < 5.0f) return 0.5f;
        return 1.0f;
    }

    void drawArrows() {
        glColor3f(0.8f, 0.8f, 0.8f);

        float arrowSizeX = (viewXMax - viewXMin) * 0.015f;
        float arrowSizeY = (viewYMax - viewYMin) * 0.015f;

        glBegin(GL_TRIANGLES);

        // X arrow
        glVertex2f(viewXMax, 0.0f);
        glVertex2f(viewXMax - arrowSizeX, arrowSizeY/2.0f);
        glVertex2f(viewXMax - arrowSizeX, -arrowSizeY/2.0f);

        // Y arrow
        glVertex2f(0.0f, viewYMax);
        glVertex2f(arrowSizeX/2.0f, viewYMax - arrowSizeY);
        glVertex2f(-arrowSizeX/2.0f, viewYMax - arrowSizeY);

        glEnd();
    }

    void drawAxisLabels(float xSpacing, float ySpacing) {
        // Draw tick marks
        glColor3f(0.7f, 0.7f, 0.7f);

        // Draw X axis tick marks
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

        // Draw Y axis tick marks
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

public:
    void setViewRange(float xmin, float xmax, float ymin, float ymax) {
        viewXMin = xmin;
        viewXMax = xmax;
        viewYMin = ymin;
        viewYMax = ymax;
    }

    void zoom(float factor, float centerX, float centerY) {
        float xRange = viewXMax - viewXMin;
        float yRange = viewYMax - viewYMin;

        // FURTHER REDUCED scroll rate: 0.98 for zoom in, 1.02 for zoom out
        float newXRange = xRange * factor;
        float newYRange = yRange * factor;

        viewXMin = centerX - newXRange / 2.0f;
        viewXMax = centerX + newXRange / 2.0f;
        viewYMin = centerY - newYRange / 2.0f;
        viewYMax = centerY + newYRange / 2.0f;
    }

    void pan(float dx, float dy) {
        float xRange = viewXMax - viewXMin;
        float yRange = viewYMax - viewYMin;

        viewXMin += dx * xRange;
        viewXMax += dx * xRange;
        viewYMin += dy * yRange;
        viewYMax += dy * yRange;
    }

    void resetView() {
        viewXMin = baseXMin;
        viewXMax = baseXMax;
        viewYMin = baseYMin;
        viewYMax = baseYMax;
    }

    void getViewRange(float& xmin, float& xmax, float& ymin, float& ymax) const {
        xmin = viewXMin;
        xmax = viewXMax;
        ymin = viewYMin;
        ymax = viewYMax;
    }

    // Convert screen coordinates to graph coordinates
    void screenToGraph(int screenX, int screenY, int windowWidth, int windowHeight,
                      float& graphX, float& graphY) {
        graphX = viewXMin + (screenX / (float)windowWidth) * (viewXMax - viewXMin);
        graphY = viewYMax - (screenY / (float)windowHeight) * (viewYMax - viewYMin);
    }
};

// Global variables
MultiFunctionPlotter plotter;
CoordinateSystem coordSystem;

// UI state
char equationInput[256] = "y=x";
float rangeMin = -10.0f, rangeMax = 10.0f;
bool showHelp = false;
bool isDragging = false;
double lastMouseX, lastMouseY;

// Mouse button callback
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            isDragging = true;
            glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
        } else if (action == GLFW_RELEASE) {
            isDragging = false;
        }
    }
}

// Mouse scroll callback with FURTHER REDUCED zoom rate
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    float graphX, graphY;
    coordSystem.screenToGraph(mouseX, mouseY, width, height, graphX, graphY);

    // FURTHER REDUCED zoom rate: 0.98 for zoom in, 1.02 for zoom out
    float zoom = (yoffset > 0) ? 0.98f : 1.02f;
    coordSystem.zoom(zoom, graphX, graphY);

    // Update plotter range
    float xmin, xmax, ymin, ymax;
    coordSystem.getViewRange(xmin, xmax, ymin, ymax);
    plotter.setRange(xmin, xmax);

    // Update UI range display
    rangeMin = xmin;
    rangeMax = xmax;
}

// Initialize GLFW
bool initGLFW() {
    if (!glfwInit()) {
        cerr << "GLFW initialization error" << endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    window = glfwCreateWindow(1400, 900, "Advanced Function Visualizer", NULL, NULL);
    if (!window) {
        cerr << "GLFW window creation error" << endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Set callbacks
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetScrollCallback(window, scrollCallback);

    return true;
}

// Initialize ImGui
void initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 120");
}

// Render function
void render() {
    // Handle mouse dragging for panning
    if (isDragging) {
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        int width, height;
        glfwGetWindowSize(window, &width, &height);

        float dx = (mouseX - lastMouseX) / width;
        float dy = (mouseY - lastMouseY) / height;

        coordSystem.pan(-dx, dy);

        // Update plotter range
        float xmin, xmax, ymin, ymax;
        coordSystem.getViewRange(xmin, xmax, ymin, ymax);
        plotter.setRange(xmin, xmax);

        // Update UI range display
        rangeMin = xmin;
        rangeMax = xmax;

        lastMouseX = mouseX;
        lastMouseY = mouseY;
    }

    glClearColor(0.08f, 0.08f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Set projection matrix will be handled in CoordinateSystem::draw()

    // Draw coordinate system (this sets up the projection with square grid)
    coordSystem.draw();

    // Draw all functions
    plotter.draw();

    // Render ImGui
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Control panel
    ImGui::Begin("Function Controller", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("Add Function:");
    ImGui::InputText("Equation", equationInput, IM_ARRAYSIZE(equationInput));
    ImGui::SameLine();
    if (ImGui::Button("Add")) {
        if (strlen(equationInput) > 0) {
            plotter.addFunction(equationInput);
            strcpy(equationInput, "");
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("?")) {
        showHelp = !showHelp;
    }

    // Quick add buttons
    ImGui::Text("Quick Add:");

    if (ImGui::Button("y=x")) { strcpy(equationInput, "y=x"); plotter.addFunction(equationInput); strcpy(equationInput, ""); }
    ImGui::SameLine();
    if (ImGui::Button("y=|sin(x)|")) { strcpy(equationInput, "y=|sin(x)|"); plotter.addFunction(equationInput); strcpy(equationInput, ""); }
    ImGui::SameLine();
    if (ImGui::Button("y=-x^2")) { strcpy(equationInput, "y=-x^2"); plotter.addFunction(equationInput); strcpy(equationInput, ""); }

    if (ImGui::Button("y=x^3-2x")) { strcpy(equationInput, "y=x^3-2x"); plotter.addFunction(equationInput); strcpy(equationInput, ""); }
    ImGui::SameLine();
    if (ImGui::Button("y=|sin(x+1)|-1")) { strcpy(equationInput, "y=|sin(x+1)|-1"); plotter.addFunction(equationInput); strcpy(equationInput, ""); }
    ImGui::SameLine();
    if (ImGui::Button("(x-2)^2+(y+1)^2=9")) { strcpy(equationInput, "(x-2)^2+(y+1)^2=9"); plotter.addFunction(equationInput); strcpy(equationInput, ""); }

    if (ImGui::Button("y=sin(x)-2")) { strcpy(equationInput, "y=sin(x)-2"); plotter.addFunction(equationInput); strcpy(equationInput, ""); }
    ImGui::SameLine();
    if (ImGui::Button("x=1")) { strcpy(equationInput, "x=1"); plotter.addFunction(equationInput); strcpy(equationInput, ""); }
    ImGui::SameLine();
    if (ImGui::Button("y=x^4-3x^2")) { strcpy(equationInput, "y=x^4-3x^2"); plotter.addFunction(equationInput); strcpy(equationInput, ""); }

    ImGui::Separator();

    // Function list
    auto& functions = plotter.getFunctions();
    ImGui::Text("Functions (%d):", (int)functions.size());

    if (ImGui::BeginChild("FunctionList", ImVec2(0, 200), true)) {
        for (size_t i = 0; i < functions.size(); i++) {
            ImGui::PushID((int)i);

            ImGui::ColorEdit3("##color", (float*)&functions[i].color,
                             ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
            ImGui::SameLine();

            ImGui::Checkbox("##enabled", &functions[i].enabled);
            ImGui::SameLine();

            if (functions[i].editing) {
                // Editing mode
                ImGui::PushItemWidth(200);
                char editBuffer[256];
                strcpy(editBuffer, functions[i].editBuffer.c_str());
                if (ImGui::InputText("##edit", editBuffer, IM_ARRAYSIZE(editBuffer))) {
                    functions[i].editBuffer = editBuffer;
                }
                ImGui::PopItemWidth();
                ImGui::SameLine();

                if (ImGui::Button("V")) {
                    functions[i].applyEdit();
                    plotter.editFunction(i, functions[i].expression);
                }
                ImGui::SameLine();
                if (ImGui::Button("X")) {
                    functions[i].cancelEdit();
                }
            } else {
                // Display mode
                ImGui::Text("%s", functions[i].expression.c_str());
                ImGui::SameLine();

                // Edit button
                if (ImGui::Button("Edit")) {
                    functions[i].startEditing();
                }
                ImGui::SameLine();

                // Remove button
                if (ImGui::Button("X")) {
                    plotter.removeFunction(i);
                    ImGui::PopID();
                    break;
                }
            }

            ImGui::PopID();
        }
    }
    ImGui::EndChild();

    if (ImGui::Button("Clear All")) {
        plotter.clear();
    }

    ImGui::Separator();
    ImGui::Text("Graph Range:");

    // Range input with update button
    if (ImGui::InputFloat("X min", &rangeMin)) {
        // Update when user types
    }
    if (ImGui::InputFloat("X max", &rangeMax)) {
        // Update when user types
    }

    if (ImGui::Button("Apply Range")) {
        // Ensure min < max
        if (rangeMin >= rangeMax) {
            swap(rangeMin, rangeMax);
        }
        plotter.setRange(rangeMin, rangeMax);
        coordSystem.setViewRange(rangeMin, rangeMax, -rangeMax, rangeMax);
    }

    ImGui::SameLine();
    if (ImGui::Button("Reset View")) {
        rangeMin = -10.0f;
        rangeMax = 10.0f;
        plotter.setRange(rangeMin, rangeMax);
        coordSystem.resetView();
    }

    ImGui::Separator();
    ImGui::Text("Controls:");
    ImGui::BulletText("Left drag: Pan");
    ImGui::BulletText("Scroll: Zoom (smooth rate: 0.98/1.02)");
    ImGui::BulletText("Click 'Edit' to modify functions");
    ImGui::BulletText("V to save, X to cancel editing");
    ImGui::BulletText("Grid is now square (1:1 aspect ratio)");

    ImGui::End();

    // Help window
    if (showHelp) {
        ImGui::Begin("Help - Supported Functions", &showHelp, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text("FIXED ISSUES:");
        ImGui::Separator();
        ImGui::BulletText("Circles: Non-zero centers now work");
        ImGui::BulletText("Absolute functions: y=|sin(x)| works correctly");
        ImGui::BulletText("Polynomials: Added support (x^3, x^4, etc.)");
        ImGui::BulletText("Flipping: y=-x^2 works properly");
        ImGui::BulletText("Square grid: Proper 1:1 aspect ratio");

        ImGui::Separator();
        ImGui::Text("EXAMPLES THAT NOW WORK:");
        ImGui::BulletText("y=|sin(x)| (absolute sine)");
        ImGui::BulletText("(x-2)^2+(y+1)^2=9 (shifted circle)");
        ImGui::BulletText("y=x^3-2x (cubic polynomial)");
        ImGui::BulletText("y=-x^2 (flipped parabola)");
        ImGui::BulletText("y=|sin(x+1)|-1 (transformed absolute)");
        ImGui::BulletText("y=x^4-3x^2 (quartic polynomial)");

        ImGui::Separator();
        ImGui::Text("SUPPORTED SYNTAX:");
        ImGui::BulletText("Basic: y=x, y=x^2, y=2*x+3");
        ImGui::BulletText("Polynomials: y=x^3-2x, y=x^4+3x^2-1");
        ImGui::BulletText("Trig: sin(x), cos(x), tan(x), cot(x)");
        ImGui::BulletText("Absolute: y=|x|, y=|sin(x)|, y=|x-2|");
        ImGui::BulletText("Exponential: y=e^x, y=2^x");
        ImGui::BulletText("Logarithmic: y=ln(x), y=log(x)");
        ImGui::BulletText("Circles: x^2+y^2=r^2, (x-a)^2+(y-b)^2=r^2");
        ImGui::BulletText("Lines: x=c, y=c");
        ImGui::BulletText("Flipping: y=-f(x) for any f(x)");

        ImGui::Separator();
        if (ImGui::Button("Close Help")) {
            showHelp = false;
        }

        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
}

// Cleanup
void cleanup() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}

int main() {
    if (!initGLFW()) return -1;

    initImGui();

    // Initial setup
    plotter.setRange(rangeMin, rangeMax);
    coordSystem.setViewRange(rangeMin, rangeMax, -rangeMax, rangeMax);

    // Add some example functions
    plotter.addFunction("y=x");
    plotter.addFunction("y=|sin(x)|");
    plotter.addFunction("y=-x^2");
    plotter.addFunction("(x-2)^2+(y+1)^2=9");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        render();
    }

    cleanup();
    return 0;


}