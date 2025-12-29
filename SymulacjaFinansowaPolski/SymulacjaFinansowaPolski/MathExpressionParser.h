#ifndef MATHEXPRESSIONPARSER_H
#define MATHEXPRESSIONPARSER_H

#include <string>
#include <vector>
#include <utility>

/**
 * Definicje typów funkcji
 */
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
    UNKNOWN // Używany także do sygnalizowania błędu parsowania/składni
};

class MathExpressionParser {
private:
    std::string expression;
    FunctionType type;
    
    // Używamy std::pair<współczynnik, wykładnik> dla wielomianów
    std::vector<std::pair<float, float>> polynomialTerms;

    float verticalLineX;
    float horizontalLineY;
    float circleCenterX, circleCenterY, circleRadius;
    bool isCircle;
    
    std::string errorMessage; // Przechowuje komunikat o błędzie

    // Funkcje narzędziowe
    std::string removeWhitespace(const std::string& str);
    std::string toLower(const std::string& str);
    bool contains(const std::string& str, const std::string& substr);
    void replaceAll(std::string& str, const std::string& from, const std::string& to);
    size_t findMatchingParen(const std::string& str, size_t start);
    
    // Walidacja i Parsowanie
    bool isValidCharacter(char c);
    void normalizeExpression(std::string& expr);
    void parseCircleEquation(const std::string& expr);
    void parsePolynomial(const std::string& expr);
    
    /**
     * Rekursywnie parsowanie i obliczanie. W przypadku błędu ustawia errorMessage.
     */
    float parseExpression(float x, const std::string& expr);
    
    void detectFunctionType();

public:
    MathExpressionParser();
    void setExpression(const std::string& expr);
    
    /**
     * Oblicza wartość funkcji. Zwraca NAN w przypadku błędu lub gdy funkcja jest linią/okręgiem.
     */
    float evaluate(float x);
    
    // Gettery
    FunctionType getType() const;
    std::string getExpression() const;
    float getVerticalLineX() const;
    float getHorizontalLineY() const;
    bool isCircleEquation() const;
    void getCircleParams(float& cx, float& cy, float& r) const;

    /**
     * Zwraca szczegółowy komunikat o błędzie. Pusty string oznacza brak błędu.
     */
    std::string getErrorMessage() const;
};

#endif // MATHEXPRESSIONPARSER_H
