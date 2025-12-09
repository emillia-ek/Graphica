#ifndef MATHEXPRESSIONPARSER_H
#define MATHEXPRESSIONPARSER_H

#include <string>
#include <vector>
#include <utility>

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
    VERTICAL_LINE,
    HORIZONTAL_LINE,
    CIRCLE,
    CONSTANT_POWER,
    UNKNOWN
};

class MathExpressionParser {
private:
    std::string expression;
    FunctionType type;
    std::vector<float> coefficients;
    float verticalLineX;
    float horizontalLineY;
    float circleCenterX, circleCenterY, circleRadius;
    bool isCircle;
    std::vector<std::pair<float, float>> polynomialTerms;

    std::string removeWhitespace(const std::string& str);
    std::string toLower(const std::string& str);
    bool contains(const std::string& str, const std::string& substr);
    void replaceAll(std::string& str, const std::string& from, const std::string& to);
    void normalizeExpression(std::string& expr);
    void parseCircleEquation(const std::string& expr);
    void parsePolynomial(const std::string& expr);
    float parseExpression(float x, const std::string& expr);
    size_t findMatchingParen(const std::string& str, size_t start);
    void detectFunctionType();

public:
    MathExpressionParser();
    void setExpression(const std::string& expr);
    float evaluate(float x);
    FunctionType getType() const;
    std::string getExpression() const;
    float getVerticalLineX() const;
    float getHorizontalLineY() const;
    bool isCircleEquation() const;
    void getCircleParams(float& cx, float& cy, float& r) const;
};

#endif