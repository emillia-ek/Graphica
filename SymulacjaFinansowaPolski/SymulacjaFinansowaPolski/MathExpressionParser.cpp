#include "MathExpressionParser.h"
#include <cmath>
#include <algorithm>
#include <cctype>
#include <stdexcept>

using namespace std;

MathExpressionParser::MathExpressionParser() : type(UNKNOWN), verticalLineX(0.0f), horizontalLineY(0.0f),
                                               circleCenterX(0.0f), circleCenterY(0.0f), circleRadius(1.0f),
                                               isCircle(false) {}

string MathExpressionParser::removeWhitespace(const string& str) {
    string result;
    remove_copy_if(str.begin(), str.end(), back_inserter(result),
                  [](char c) { return isspace(c); });
    return result;
}

string MathExpressionParser::toLower(const string& str) {
    string result = str;
    transform(result.begin(), result.end(), result.begin(),
             [](unsigned char c) { return tolower(c); });
    return result;
}

bool MathExpressionParser::contains(const string& str, const string& substr) {
    return str.find(substr) != string::npos;
}

void MathExpressionParser::replaceAll(string& str, const string& from, const string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

void MathExpressionParser::normalizeExpression(string& expr) {
    string original = expr;

    // Check for circle equation
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
        string afterEqual = expr.substr(2);
        if (!contains(afterEqual, "x") &&
            !contains(afterEqual, "sin") &&
            !contains(afterEqual, "cos") &&
            !contains(afterEqual, "tan") &&
            !contains(afterEqual, "cot") &&
            !contains(afterEqual, "log") &&
            !contains(afterEqual, "ln") &&
            !contains(afterEqual, "exp") &&
            !contains(afterEqual, "e^")) {
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

    // Remove y= if present (but not for special cases)
    if (expr.find("y=") == 0 && type != VERTICAL_LINE && type != HORIZONTAL_LINE) {
        expr = expr.substr(2);
    }

    expr = removeWhitespace(expr);
}

void MathExpressionParser::parseCircleEquation(const string& expr) {
    isCircle = true;
    type = CIRCLE;

    circleCenterX = 0.0f;
    circleCenterY = 0.0f;
    circleRadius = 1.0f;

    try {
        string cleanExpr = removeWhitespace(toLower(expr));
        size_t eqPos = cleanExpr.find('=');
        if (eqPos == string::npos) return;

        string rightSide = cleanExpr.substr(eqPos + 1);
        float radiusSquared = stof(rightSide);
        circleRadius = sqrt(radiusSquared);

        string leftSide = cleanExpr.substr(0, eqPos);

        // Extract x part
        size_t xStart = leftSide.find("(x");
        if (xStart != string::npos) {
            size_t xEnd = leftSide.find(")^2", xStart);
            if (xEnd != string::npos) {
                string xExpr = leftSide.substr(xStart + 1, xEnd - xStart - 1);
                if (xExpr.length() > 1) {
                    char op = xExpr[1];
                    float value = stof(xExpr.substr(2));
                    if (op == '-') {
                        circleCenterX = value;
                    } else if (op == '+') {
                        circleCenterX = -value;
                    }
                }
            }
        } else if (contains(leftSide, "x^2")) {
            circleCenterX = 0.0f;
        }

        // Extract y part
        size_t yStart = leftSide.find("(y");
        if (yStart != string::npos) {
            size_t yEnd = leftSide.find(")^2", yStart);
            if (yEnd != string::npos) {
                string yExpr = leftSide.substr(yStart + 1, yEnd - yStart - 1);
                if (yExpr.length() > 1) {
                    char op = yExpr[1];
                    float value = stof(yExpr.substr(2));
                    if (op == '-') {
                        circleCenterY = value;
                    } else if (op == '+') {
                        circleCenterY = -value;
                    }
                }
            }
        } else if (contains(leftSide, "y^2")) {
            circleCenterY = 0.0f;
        }

    } catch (...) {
        circleCenterX = 0.0f;
        circleCenterY = 0.0f;
        circleRadius = 1.0f;
    }
}

void MathExpressionParser::parsePolynomial(const string& expr) {
    polynomialTerms.clear();
    string cleanExpr = expr;

    // Handle unary minus at the beginning
    if (cleanExpr[0] == '-') {
        cleanExpr = "0" + cleanExpr;
    }

    // Replace multiple minus signs
    replaceAll(cleanExpr, "+-", "-");
    replaceAll(cleanExpr, "-+", "-");

    // Parse terms
    vector<string> terms;
    string currentTerm;
    int parenCount = 0;

    for (size_t i = 0; i < cleanExpr.length(); i++) {
        char c = cleanExpr[i];
        if (c == '(') parenCount++;
        else if (c == ')') parenCount--;

        if (parenCount == 0 && (c == '+' || c == '-') && i > 0 && cleanExpr[i-1] != '^' && cleanExpr[i-1] != 'e') {
            if (!currentTerm.empty()) {
                terms.push_back(currentTerm);
            }
            currentTerm = c;
        } else {
            currentTerm += c;
        }
    }
    if (!currentTerm.empty()) {
        terms.push_back(currentTerm);
    }

    for (string& term : terms) {
        if (term[0] == '+') term = term.substr(1);

        if (contains(term, "x")) {
            size_t xPos = term.find('x');
            float coefficient = 1.0f;
            float exponent = 1.0f;

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

float MathExpressionParser::parseExpression(float x, const string& expr) {
    if (expr.empty()) return 0.0f;

    string trimmed = expr;

    // Handle unary minus at the beginning
    if (trimmed[0] == '-') {
        trimmed = "0" + trimmed;
    }

    // Remove outer parentheses
    while (trimmed.front() == '(' && trimmed.back() == ')') {
        trimmed = trimmed.substr(1, trimmed.length() - 2);
    }

    // Check for addition/subtraction
    int parenCount = 0;
    size_t opPos = string::npos;
    char op = '+';

    for (size_t i = 0; i < trimmed.length(); i++) {
        char c = trimmed[i];
        if (c == '(') parenCount++;
        else if (c == ')') parenCount--;
        else if (parenCount == 0 && (c == '+' || c == '-') && i > 0) {
            char prev = trimmed[i-1];
            if (prev != 'e' && prev != 'E' &&
                !(i > 1 && (trimmed[i-2] == 'e' || trimmed[i-2] == 'E')) &&
                prev != '*' && prev != '/' && prev != '^') {
                opPos = i;
                op = c;
                break;
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

    // Check for multiplication/division
    parenCount = 0;
    opPos = string::npos;

    // First check for division
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

    // Then check for multiplication
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

    // FIXED: Check for implied multiplication for sin(2x)
    // Look for patterns like: number followed by letter, number followed by '(', letter followed by '('
    for (size_t i = 0; i < trimmed.length() - 1; i++) {
        char current = trimmed[i];
        char next = trimmed[i + 1];

        // Cases for implied multiplication:
        // 1. Number followed by letter (e.g., 2x, 3sin)
        // 2. Number followed by '(' (e.g., 2(x+1))
        // 3. Letter followed by '(' (e.g., x(2), but this is rare)
        // 4. ')' followed by letter or '(' (e.g., (x+2)(x+3))
        // 5. ')' followed by number (e.g., (x+2)3)

        bool shouldMultiply = false;

        if (isdigit(current) && (isalpha(next) || next == '(')) {
            shouldMultiply = true;  // 2x, 2sin, 2(x+1)
        } else if (current == ')' && (isalpha(next) || isdigit(next) || next == '(')) {
            shouldMultiply = true;  // (x+2)(x+3), (x+2)3
        } else if (isalpha(current) && next == '(') {
            shouldMultiply = true;  // f(x) style, though rare
        } else if (current == 'x' && (isdigit(next) || isalpha(next) || next == '(')) {
            shouldMultiply = true;  // x2, xsin
        }

        if (shouldMultiply) {
            string left = trimmed.substr(0, i + 1);
            string right = trimmed.substr(i + 1);
            return parseExpression(x, left) * parseExpression(x, right);
        }
    }

    // Check for power
    size_t maxPos = string::npos;
    parenCount = 0;
    for (size_t i = 0; i < trimmed.length(); i++) {
        char c = trimmed[i];
        if (c == '(') parenCount++;
        else if (c == ')') parenCount--;
        else if (parenCount == 0 && c == '^') {
            maxPos = i;
        }
    }

    if (maxPos != string::npos) {
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
        if (fabs(cos(val)) < 0.001f) return NAN;
        return tan(val);
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

size_t MathExpressionParser::findMatchingParen(const string& str, size_t start) {
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

void MathExpressionParser::detectFunctionType() {
    string expr = expression;

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
        !contains(expr, "log") && !contains(expr, "ln") && !contains(expr, "exp")) {
        type = LINEAR;
        return;
    }

    // Check for polynomial (contains x^3, x^4, etc.)
    if (contains(expr, "x^")) {
        parsePolynomial(expr);
        return;
    }

    type = UNKNOWN;
}

void MathExpressionParser::setExpression(const string& expr) {
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

float MathExpressionParser::evaluate(float x) {
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
        if (type == CONSTANT_POWER) {
            return 1.0f;
        }

        if (type == POLYNOMIAL && !polynomialTerms.empty()) {
            float result = 0.0f;
            for (const auto& term : polynomialTerms) {
                if (term.second == 0) {
                    result += term.first;
                } else {
                    result += term.first * pow(x, term.second);
                }
            }
            return result;
        }

        return parseExpression(x, expr);

    } catch (const exception& e) {
        return NAN;
    }
}

FunctionType MathExpressionParser::getType() const { return type; }
string MathExpressionParser::getExpression() const { return expression; }
float MathExpressionParser::getVerticalLineX() const { return verticalLineX; }
float MathExpressionParser::getHorizontalLineY() const { return horizontalLineY; }
bool MathExpressionParser::isCircleEquation() const { return isCircle; }

void MathExpressionParser::getCircleParams(float& cx, float& cy, float& r) const {
    cx = circleCenterX;
    cy = circleCenterY;
    r = circleRadius;
}