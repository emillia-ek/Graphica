#ifndef MATHEXPRESSIONPARSER_H
#define MATHEXPRESSIONPARSER_H

#include <string>
#include <vector>

enum FunctionType {
    UNKNOWN, LINEAR, QUADRATIC, POLYNOMIAL, SIN, COS, TAN, COT,
    LOGARITHMIC, EXPONENTIAL, HORIZONTAL_LINE, VERTICAL_LINE, CIRCLE,
    SPHERE_3D, CONE_3D
};

class MathExpressionParser {
public:
    MathExpressionParser();

    void setExpression(const std::string& expr);
    float evaluate(float x);
    float evaluate(float x, float y);

    FunctionType getType() const;
    std::string getExpression() const;
    float getVerticalLineX() const;
    float getHorizontalLineY() const;
    bool isCircleEquation() const;
    bool is3DFunction() const;
    void getCircleParams(float& cx, float& cy, float& r) const;
    float getSphereRadius() const;
    std::string getErrorMessage() const;

private:
    std::string removeWhitespace(const std::string& str);
    std::string toLower(const std::string& str);
    bool contains(const std::string& str, const std::string& substr);
    void replaceAll(std::string& str, const std::string& from, const std::string& to);
    bool isValidCharacter(char c);

    void normalizeExpression(std::string& expr);
    void parseCircleEquation(const std::string& expr);
    void parseGeneralCircleEquation(const std::string& expr); // DODAJ TĘ LINIĘ
    void parsePolynomial(const std::string& expr);
    void detectFunctionType();

    float parseExpression(float x, float y, const std::string& expr);
    size_t findMatchingParen(const std::string& str, size_t start);

    std::string expression;
    std::string errorMessage;
    FunctionType type;

    // Linie
    float verticalLineX;
    float horizontalLineY;

    // Okrąg
    float circleCenterX;
    float circleCenterY;
    float circleRadius;
    bool isCircle;
    bool is3D;
    float sphereRadius;

    // Wielomian
    struct PolynomialTerm {
        float coefficient;
        float exponent;
        PolynomialTerm(float c, float e) : coefficient(c), exponent(e) {}
    };
    std::vector<PolynomialTerm> polynomialTerms;
};

#endif