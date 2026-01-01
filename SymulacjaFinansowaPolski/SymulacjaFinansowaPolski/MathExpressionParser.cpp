#include "MathExpressionParser.h"
#include <cmath>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <sstream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_E
#define M_E 2.71828182845904523536
#endif

using namespace std;
MathExpressionParser::MathExpressionParser() : type(UNKNOWN), verticalLineX(0.0f), horizontalLineY(0.0f),
                                             circleCenterX(0.0f), circleCenterY(0.0f), circleRadius(1.0f),
                                             isCircle(false), errorMessage("") {}

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

// Walidacja i Normalizacja

bool MathExpressionParser::isValidCharacter(char c) {
    return isalnum(c) || isspace(c) || c == '+' || c == '-' || c == '*' || c == '/' ||
           c == '^' || c == '(' || c == ')' || c == '.' || c == '=' || c == '|';
}

void MathExpressionParser::normalizeExpression(string& expr) {
    
    //Obsługa wartości bezwzględnej
    string result = "";
        bool opening = true;
        for (size_t i = 0; i < expr.length(); ++i) {
            if (expr[i] == '|') {
                if (opening) result += "abs(";
                else result += ")";
                opening = !opening;
            } else {
                result += expr[i];
            }
        }
        if (!opening) {
            result += ")";
        }
        expr = result;
    
    //Wykrywanie okręgu
    if ((contains(expr, "(x") && contains(expr, ")^2") &&
        contains(expr, "(y") && contains(expr, ")^2") &&
        contains(expr, "=")) ||
        (contains(expr, "x^2") && contains(expr, "y^2") && contains(expr, "="))) {
        parseCircleEquation(expr);
        return;
    }

    //Normalizacja y= / f(x)=
    if (expr.find("f(x)=") == 0) {
        expr = "y=" + expr.substr(5);
    } else if (expr.find("f(x)") == 0 && expr.length() > 4) {
        expr = "y=" + expr.substr(4);
    }

    //Linie poziome y = stała
    if (expr.find("y=") == 0) {
        string afterEqual = expr.substr(2);
        if (!contains(afterEqual, "x") &&
            !contains(afterEqual, "sin") && !contains(afterEqual, "cos") &&
            !contains(afterEqual, "tan") && !contains(afterEqual, "cot") &&
            !contains(afterEqual, "log") && !contains(afterEqual, "ln") &&
            !contains(afterEqual, "abs") && !contains(afterEqual, "exp")) {
            try {
                horizontalLineY = stof(afterEqual);
                type = HORIZONTAL_LINE;
                expr = "horizontal";
                return;
            } catch (...) { /* Kontynuuj parsowanie */ }
        }
    }

    // 5. Linie pionowe x = stała
    if (expr.find("x=") == 0) {
        size_t eqPos = expr.find('=');
        if (eqPos != string::npos) {
            string afterEqual = expr.substr(eqPos + 1);
            if (!contains(afterEqual, "x") && !contains(afterEqual, "y") &&
                !contains(afterEqual, "sin") && !contains(afterEqual, "cos")) {
                try {
                    verticalLineX = stof(afterEqual);
                    type = VERTICAL_LINE;
                    expr = "vertical";
                    return;
                } catch (...) { /* Kontynuuj parsowanie */ }
            }
        }
    }

    // 6. Zamiana synonimów
    replaceAll(expr, "tg", "tan");
    replaceAll(expr, "ctg", "cot");

    // 7. Usunięcie "y=" (dla normalnej funkcji)
    if (expr.find("y=") == 0 && type != VERTICAL_LINE && type != HORIZONTAL_LINE) {
        expr = expr.substr(2);
    }
}

void MathExpressionParser::parseCircleEquation(const string& expr) {
    isCircle = true;
    type = CIRCLE;
    circleCenterX = 0.0f;
    circleCenterY = 0.0f;
    circleRadius = 0.0f;

    try {
        size_t eqPos = expr.find('=');
        if (eqPos == string::npos) return;

        string rightSide = expr.substr(eqPos + 1);
        float radiusSquared = stof(rightSide);
        if (radiusSquared >= 0) {
            circleRadius = sqrt(radiusSquared);
        } else {
            errorMessage = "Blad: Ujemny promien ($R^2 < 0$) dla rownania okregu.";
            isCircle = false;
            type = UNKNOWN;
            return;
        }

        string leftSide = expr.substr(0, eqPos);

        size_t xStart = leftSide.find("(x");
        if (xStart != string::npos) {
            size_t xEnd = leftSide.find(")^2", xStart);
            if (xEnd != string::npos) {
                string xExpr = leftSide.substr(xStart + 1, xEnd - xStart - 1);
                if (xExpr.length() > 1) {
                    char op = xExpr[1];
                    float value = stof(xExpr.substr(2));
                    circleCenterX = (op == '-') ? value : -value;
                }
            }
        } else if (contains(leftSide, "x^2")) {
            circleCenterX = 0.0f;
        }

        size_t yStart = leftSide.find("(y");
        if (yStart != string::npos) {
            size_t yEnd = leftSide.find(")^2", yStart);
            if (yEnd != string::npos) {
                string yExpr = leftSide.substr(yStart + 1, yEnd - yStart - 1);
                if (yExpr.length() > 1) {
                    char op = yExpr[1];
                    float value = stof(yExpr.substr(2));
                    circleCenterY = (op == '-') ? value : -value;
                }
            }
        } else if (contains(leftSide, "y^2")) {
            circleCenterY = 0.0f;
        }

    } catch (...) {
        errorMessage = "Blad parsowania rownania okregu.";
        isCircle = false;
        type = UNKNOWN;
    }
}

void MathExpressionParser::parsePolynomial(const string& expr) {
    polynomialTerms.clear();
    string cleanExpr = expr;

    if (cleanExpr[0] == '-') {
        cleanExpr = "0" + cleanExpr;
    }
    
    replaceAll(cleanExpr, "+-", "-");
    replaceAll(cleanExpr, "-+", "-");

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
            } catch (...) {}
        }
    }
    type = POLYNOMIAL;
}

float MathExpressionParser::parseExpression(float x, const string& expr) {
    if (expr.empty()) return 0.0f;
    if (!errorMessage.empty()) return NAN;

    string trimmed = expr;

    if (trimmed[0] == '-') {
        trimmed = "0" + trimmed;
    }
    while (trimmed.length() > 2 && trimmed.front() == '(' && trimmed.back() == ')') {
        size_t match = findMatchingParen(trimmed, 0);
        if (match == trimmed.length() - 1) {
            trimmed = trimmed.substr(1, trimmed.length() - 2);
        } else {
            break;
        }
    }
    
    // --- 1. Dodawanie/Odejmowanie ---
    int parenCount = 0;
    size_t opPos = string::npos;
    char op = '+';

    for (size_t i = trimmed.length() - 1; i > 0; i--) {
        char c = trimmed[i];
        if (c == ')') parenCount++;
        else if (c == '(') parenCount--;
        else if (parenCount == 0 && (c == '+' || c == '-')) {
            char prev = trimmed[i-1];
            if (prev != 'e' && prev != 'E' && prev != '*' && prev != '/' && prev != '^' && prev != '(') {
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
        if (!errorMessage.empty() || isnan(leftVal) || isnan(rightVal)) return NAN;
        return (op == '+') ? leftVal + rightVal : leftVal - rightVal;
    }

    // --- 2. Mnożenie/Dzielenie ---
    parenCount = 0;
    opPos = string::npos;
    char mulDivOp = ' ';

    for (size_t i = trimmed.length() - 1; i > 0; i--) {
        char c = trimmed[i];
        if (c == ')') parenCount++;
        else if (c == '(') parenCount--;
        else if (parenCount == 0 && (c == '*' || c == '/')) {
            opPos = i;
            mulDivOp = c;
            break;
        }
    }

    if (opPos != string::npos) {
        string left = trimmed.substr(0, opPos);
        string right = trimmed.substr(opPos + 1);
        float leftVal = parseExpression(x, left);
        float rightVal = parseExpression(x, right);
        
        if (!errorMessage.empty() || isnan(leftVal) || isnan(rightVal)) return NAN;

        if (mulDivOp == '/') {
                float leftVal = parseExpression(x, left);
                float rightVal = parseExpression(x, right);
                if (fabs(rightVal) < 0.000001f) {
                    errorMessage = "Blad matematyczny: Dzielenie przez zero.";
                    return NAN;
                }
                return leftVal / rightVal;
            }
    }
    
    // Mnożenie implikowane
    int pCount = 0;
        for (size_t i = 0; i < trimmed.length() - 1; i++) {
            char c = trimmed[i];
            if (c == '(') pCount++;
            else if (c == ')') pCount--;

            // Mnożenie sprawdzamy poza nawiasami funkcji
            if (pCount == 0) {
                char current = trimmed[i];
                char next = trimmed[i + 1];

                bool shouldMultiply = (isdigit(current) && (isalpha(next) || next == '(')) ||
                                     (current == ')' && (isdigit(next) || isalpha(next) || next == '(')) ||
                                     (current == 'x' && (isdigit(next) || isalpha(next) || next == '('));

                if (shouldMultiply) {
                    // Sprawdzenie, by nie rozbijać nazw funkcji (np. s-in)
                    if (isalpha(current) && isalpha(next)) continue;

                    string left = trimmed.substr(0, i + 1);
                    string right = trimmed.substr(i + 1);
                    return parseExpression(x, left) * parseExpression(x, right);
                }
            }
        }
    // 4. Potęgowanie
    size_t maxPos = string::npos;
    parenCount = 0;
    for (size_t i = trimmed.length() - 1; i > 0; i--) {
        char c = trimmed[i];
        if (c == ')') parenCount++;
        else if (c == '(') parenCount--;
        else if (parenCount == 0 && c == '^') {
            maxPos = i;
            break;
        }
    }

    if (maxPos != string::npos) {
        string left = trimmed.substr(0, maxPos);
        string right = trimmed.substr(maxPos + 1);
        float base = parseExpression(x, left);
        float exponent = parseExpression(x, right);
        
        if (!errorMessage.empty() || isnan(base) || isnan(exponent)) return NAN;

        if (base < 0 && fabs(exponent - round(exponent)) > 0.0001f) {
            errorMessage = "Blad matematyczny: Potega niecalkowita z liczby ujemnej.";
            return NAN;
        }
        return pow(base, exponent);
    }
    
    // 5. Funkcje unarne
    if (trimmed.find("ln(") == 0 && trimmed.back() == ')') {
            size_t match = findMatchingParen(trimmed, 2);
            if (match == trimmed.length() - 1) {
                float val = parseExpression(x, trimmed.substr(3, trimmed.length() - 4));
                if (val <= 0) { errorMessage = "Blad: logarytm <= 0"; return NAN; }
                return log(val);
            }
        }

        if (trimmed.find("log(") == 0 && trimmed.back() == ')') {
            size_t match = findMatchingParen(trimmed, 3);
            if (match == trimmed.length() - 1) {
                float val = parseExpression(x, trimmed.substr(4, trimmed.length() - 5));
                if (val <= 0) { errorMessage = "Blad: log() tylko dla liczb > 0."; return NAN; }
                return log10(val);
            }
        }

        if (trimmed.find("tan(") == 0 && trimmed.back() == ')') {
            size_t match = findMatchingParen(trimmed, 3);
            if (match == trimmed.length() - 1) {
                float val = parseExpression(x, trimmed.substr(4, trimmed.length() - 5));
                if (fabs(cos(val)) < 0.0001f) { errorMessage = "Blad: Asymptota tangensa."; return NAN; }
                return tan(val);
            }
        }

        if (trimmed.find("sin(") == 0 && trimmed.back() == ')') {
            size_t match = findMatchingParen(trimmed, 3);
            if (match == trimmed.length() - 1) {
                return sin(parseExpression(x, trimmed.substr(4, trimmed.length() - 5)));
            }
        }

        if (trimmed.find("cos(") == 0 && trimmed.back() == ')') {
            size_t match = findMatchingParen(trimmed, 3);
            if (match == trimmed.length() - 1) {
                return cos(parseExpression(x, trimmed.substr(4, trimmed.length() - 5)));
            }
        }

    if (trimmed.find("abs(") == 0 && trimmed.back() == ')') {
            size_t match = findMatchingParen(trimmed, 3);
            if (match == trimmed.length() - 1) {
                string content = trimmed.substr(4, trimmed.length() - 5);
                return fabs(parseExpression(x, content));
            }
        }
    
        if (trimmed.find("exp(") == 0 && trimmed.back() == ')') {
            size_t match = findMatchingParen(trimmed, 3);
            if (match == trimmed.length() - 1) {
                return exp(parseExpression(x, trimmed.substr(4, trimmed.length() - 5)));
            }
        }
        
        if (trimmed.find("e^") == 0) {
            return exp(parseExpression(x, trimmed.substr(2)));
        }

    if (trimmed == "x") return x;
    if (trimmed == "-x") return -x;
    if (trimmed == "e") return M_E;
    if (trimmed == "pi") return M_PI;

    try {
        return stof(trimmed);
    } catch (...) {
        errorMessage = "Blad parsowania: Nieznany symbol '" + trimmed + "'.";
        return NAN;
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
    if (type != UNKNOWN) return;
    string expr = expression;
    if (isCircle) return;
    if (!contains(expr, "x")) { type = HORIZONTAL_LINE; return; }
    if (contains(expr, "sin(")) { type = SIN; return; }
    if (contains(expr, "cos(")) { type = COS; return; }
    if (contains(expr, "tan(")) { type = TAN; return; }
    if (contains(expr, "cot(")) { type = COT; return; }
    if (contains(expr, "log") || contains(expr, "ln(")) { type = LOGARITHMIC; return; }
    if (contains(expr, "x^2") && !contains(expr, "x^3")) { type = QUADRATIC; return; }
    if (contains(expr, "x")) { parsePolynomial(expr); }
    if (type == UNKNOWN) type = LINEAR;
}

void MathExpressionParser::setExpression(const string& expr) {
    verticalLineX = 0.0f;
    horizontalLineY = 0.0f;
    isCircle = false;
    polynomialTerms.clear();
    type = UNKNOWN;
    errorMessage = "";

    if (expr.empty()) {
        errorMessage = "Wpisz rownanie funkcji.";
        return;
    }

    string processed = removeWhitespace(toLower(expr));

    // Sprawdzenie nawiasów i modułów przed normalizacją
    int pipeCount = 0;
    for (char c : processed) if (c == '|') pipeCount++;
    if (pipeCount % 2 != 0) {
        errorMessage = "Blad: Niezamkniete znaki wartosci bezwzglednej |.";
        return;
    }

    normalizeExpression(processed);
    expression = processed;

    // Sprawdzenie nawiasów po normalizacji
    int parenCount = 0;
    for (char c : expression) {
        if (c == '(') parenCount++;
        else if (c == ')') parenCount--;
    }
    if (parenCount != 0) {
        errorMessage = "Blad: Niezamkniete nawiasy.";
        return;
    }

    detectFunctionType();
    
}

float MathExpressionParser::evaluate(float x) {
    if (!errorMessage.empty() && errorMessage.find("Blad matematyczny") == string::npos &&
        errorMessage.find("Blad:") == string::npos) {
        return NAN;
    }
    //Czyścimy błędy matematyczne przed każdym punktem x
    errorMessage = "";

    float result = parseExpression(x, expression);
    if (!errorMessage.empty()) {
        return NAN;
    }

    return result;
}

FunctionType MathExpressionParser::getType() const { return type; }
string MathExpressionParser::getExpression() const { return expression; }
float MathExpressionParser::getVerticalLineX() const { return verticalLineX; }
float MathExpressionParser::getHorizontalLineY() const { return horizontalLineY; }
bool MathExpressionParser::isCircleEquation() const { return isCircle; }
void MathExpressionParser::getCircleParams(float& cx, float& cy, float& r) const { cx = circleCenterX; cy = circleCenterY; r = circleRadius; }
string MathExpressionParser::getErrorMessage() const { return errorMessage; }
