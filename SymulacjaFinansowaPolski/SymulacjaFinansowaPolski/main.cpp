#include <iostream>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm> // Dla std::minmax_element, std::clamp

// OpenGL i GLFW headers
#include <GLFW/glfw3.h>
#include "imconfig.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// Makro dla liczby PI
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// --- Deklaracje struktur i wyliczeń ---

// Typy funkcji
enum class FunctionType {
    Linear,
    Quadratic,
    Sinus,
    Exponential,
    Logarithmic,
    Tangent, // Nowa funkcja
    SquareRoot // Nowa funkcja
};

// Struktura przechowująca parametry funkcji
struct FunctionParams {
    float p1 = 1.0f; // Współczynnik 'a'
    float p2 = 1.0f; // Współczynnik 'b'
    float p3 = 0.0f; // Współczynnik 'c'
    ImVec4 color = ImVec4(1.0f, 0.5f, 0.2f, 1.0f); // Domyślny kolor - pomarańczowy
    FunctionType type = FunctionType::Linear;
    std::string label = "Wykres 1";
    bool isVisible = true;
};

// Struktura przechowująca punkt na wykresie
struct Point {
    float x, y;
    Point(float x, float y) : x(x), y(y) {}
};

// --- Klasa zarządzająca wykresem ---

class PlotGenerator {
private:
    float xMin, xMax;
    int resolution = 500; // Zwiększona rozdzielczość

public:
    PlotGenerator() : xMin(-10.0f), xMax(10.0f) {}

    void setRange(float min, float max) {
        xMin = min;
        xMax = max;
    }

    // Funkcja obliczająca wartość Y dla danego typu i parametrów
    float calculateY(FunctionType type, float x, const FunctionParams& params) {
        switch (type) {
            case FunctionType::Linear:
                return params.p1 * x + params.p3; // y = a*x + c
            case FunctionType::Quadratic:
                return params.p1 * x * x + params.p2 * x + params.p3; // y = a*x^2 + b*x + c
            case FunctionType::Sinus:
                return params.p1 * std::sin(params.p2 * x + params.p3); // y = a*sin(b*x + c)
            case FunctionType::Exponential:
                return params.p1 * std::exp(params.p2 * x); // y = a*exp(b*x)
            case FunctionType::Logarithmic:
                if (params.p2 * x + params.p3 > 0)
                    return params.p1 * std::log(params.p2 * x + params.p3); // y = a*ln(b*x + c)
                return NAN; // poza dziedziną
            case FunctionType::Tangent:
                // Zapewnienie, że x nie jest blisko asymptoty (np. PI/2 + k*PI)
                if (std::fmod(params.p2 * x + params.p3, M_PI) > M_PI / 2.0f - 0.01f ||
                    std::fmod(params.p2 * x + params.p3, M_PI) < -M_PI / 2.0f + 0.01f) {
                    return NAN;
                }
                return params.p1 * std::tan(params.p2 * x + params.p3); // y = a*tan(b*x + c)
            case FunctionType::SquareRoot:
                if (params.p2 * x + params.p3 >= 0)
                    return params.p1 * std::sqrt(params.p2 * x + params.p3); // y = a*sqrt(b*x + c)
                return NAN; // poza dziedziną
        }
        return 0.0f;
    }

    // Generowanie punktów dla pojedynczej funkcji
    std::vector<Point> generatePlot(const FunctionParams& params) {
        std::vector<Point> points;
        float step = (xMax - xMin) / resolution;
        
        for (int i = 0; i <= resolution; ++i) {
            float x = xMin + i * step;
            float y = calculateY(params.type, x, params);
            // Dodajemy tylko poprawne punkty (nie NAN)
            if (!std::isnan(y)) {
                points.emplace_back(x, y);
            }
        }
        return points;
    }
};

// --- Klasa zarządzająca renderowaniem ---

class PlotRenderer {
private:
    float xMin, xMax, yMin, yMax;
    std::vector<Point> allPoints;

public:
    PlotRenderer() : xMin(-10.0f), xMax(10.0f), yMin(-10.0f), yMax(10.0f) {}

    // Rysowanie wykresów
    void drawPlots(const std::vector<std::vector<Point>>& plots, const std::vector<FunctionParams>& paramsList) {
        allPoints.clear();

        // Zebranie wszystkich punktów i ustalenie zakresu Y (do auto-scalingu)
        for (const auto& plot : plots) {
            allPoints.insert(allPoints.end(), plot.begin(), plot.end());
        }

        // Auto-scaling Y
        if (!allPoints.empty()) {
            auto [min_it, max_it] = std::minmax_element(allPoints.begin(), allPoints.end(),
                                                        [](const Point& a, const Point& b) { return a.y < b.y; });
            float newYMin = min_it->y;
            float newYMax = max_it->y;

            // Dodanie małego marginesu i ograniczenie, aby nie "oszalały"
            float margin = std::max(1.0f, (newYMax - newYMin) * 0.1f);
            yMin = newYMin - margin;
            yMax = newYMax + margin;

            // Zabezpieczenie przed zbyt dużymi wartościami (można dostosować)
            yMin = std::clamp(yMin, -1000.0f, 1000.0f);
            yMax = std::clamp(yMax, -1000.0f, 1000.0f);

            // Jeśli min i max są zbyt blisko siebie, ustal domyślny zakres
            if (std::abs(yMax - yMin) < 0.1f) {
                yMin -= 5.0f;
                yMax += 5.0f;
            }
        } else {
            yMin = -10.0f;
            yMax = 10.0f;
        }

        // Ustawienie macierzy projekcji
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(xMin, xMax, yMin, yMax, -1.0f, 1.0f);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // Rysowanie układu współrzędnych i siatki
        drawCoordinateSystem();

        // Rysowanie wszystkich wykresów
        for (size_t i = 0; i < plots.size(); ++i) {
            if (paramsList[i].isVisible) {
                drawSinglePlot(plots[i], paramsList[i].color);
            }
        }
    }

    // Rysowanie pojedynczego wykresu
    void drawSinglePlot(const std::vector<Point>& points, const ImVec4& color) {
        if (points.empty()) return;

        glColor3f(color.x, color.y, color.z);
        glLineWidth(2.0f); // Grubsza linia

        glBegin(GL_LINE_STRIP);
        for (const auto& point : points) {
            glVertex2f(point.x, point.y);
        }
        glEnd();
        glLineWidth(1.0f);
    }

    // Rysowanie osi i siatki
    void drawCoordinateSystem() {
        // Rysowanie siatki
        glColor3f(0.2f, 0.2f, 0.2f); // Ciemniejsza siatka
        glBegin(GL_LINES);

        float step = (xMax - xMin) > 20.0f ? 5.0f : 1.0f; // Dynamiczny krok siatki

        // Linie poziome
        for (float y = std::floor(yMin / step) * step; y <= yMax; y += step) {
            if (std::fabs(y) > 0.01f) {
                glVertex2f(xMin, y);
                glVertex2f(xMax, y);
            }
        }

        // Linie pionowe
        for (float x = std::floor(xMin / step) * step; x <= xMax; x += step) {
            if (std::fabs(x) > 0.01f) {
                glVertex2f(x, yMin);
                glVertex2f(x, yMax);
            }
        }
        glEnd();

        // Rysowanie osi
        glColor3f(0.8f, 0.8f, 0.8f); // Jaśniejsze osie
        glBegin(GL_LINES);
        // Oś X
        float x_axis_y = std::clamp(0.0f, yMin, yMax);
        glVertex2f(xMin, x_axis_y);
        glVertex2f(xMax, x_axis_y);
        // Oś Y
        float y_axis_x = std::clamp(0.0f, xMin, xMax);
        glVertex2f(y_axis_x, yMin);
        glVertex2f(y_axis_x, yMax);
        glEnd();

        // Rysowanie podziałek (Tyki) na osi X - uproszczone, ImGui renderuje tekst
        // Rysowanie podziałek (Tyki) na osi Y - uproszczone, ImGui renderuje tekst
    }

    // Gettery do zakresów
    void setXRange(float min, float max) { xMin = min; xMax = max; }
    float getXMin() const { return xMin; }
    float getXMax() const { return xMax; }
    float getYMin() const { return yMin; }
    float getYMax() const { return yMax; }
};

// --- Główna klasa aplikacji (kontroler) ---

class FunctionVisualizer {
private:
    GLFWwindow* window = nullptr;
    PlotGenerator generator;
    PlotRenderer renderer;
    std::vector<FunctionParams> functionList; // Lista funkcji do narysowania
    const char* functionNames[7] = { "Liniowa (a*x + c)", "Kwadratowa (a*x² + b*x + c)", "Sinus (a*sin(b*x + c))",
                                     "Wykładnicza (a*exp(b*x))", "Logarytmiczna (a*ln(b*x + c))",
                                     "Tangens (a*tan(b*x + c))", "Pierwiastek (a*sqrt(b*x + c))" };
    
    // Zmienne do Pan/Zoom
    float zoomLevel = 1.0f;
    float panX = 0.0f, panY = 0.0f;
    double lastMouseX, lastMouseY;
    bool isPanning = false;

public:
    FunctionVisualizer() {
        functionList.emplace_back(FunctionParams{1.0f, 0.0f, 0.0f, ImVec4(1.0f, 0.5f, 0.2f, 1.0f), FunctionType::Linear, "Liniowa", true});
    }

    bool init() {
        if (!initGLFW()) return false;
        initImGui();
        setupCallbacks();
        return true;
    }

    void run() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            render();
        }
    }

    void cleanup() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
    }

private:
    // --- Inicjalizacja ---
    bool initGLFW() {
        if (!glfwInit()) {
            std::cerr << "Błąd inicjalizacji GLFW" << std::endl;
            return false;
        }
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        window = glfwCreateWindow(1200, 800, "Wizualizator Funkcji (Wypasiony)", NULL, NULL);
        if (!window) {
            std::cerr << "Błąd tworzenia okna GLFW" << std::endl;
            glfwTerminate();
            return false;
        }
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);
        return true;
    }

    void initImGui() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 120");
    }
    
    // --- Obsługa zdarzeń (Pan/Zoom) ---
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
        FunctionVisualizer* app = static_cast<FunctionVisualizer*>(glfwGetWindowUserPointer(window));
        if (app && button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                app->isPanning = true;
                glfwGetCursorPos(window, &app->lastMouseX, &app->lastMouseY);
            } else if (action == GLFW_RELEASE) {
                app->isPanning = false;
            }
        }
        ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods); // Użycie ImGui
    }

    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
        FunctionVisualizer* app = static_cast<FunctionVisualizer*>(glfwGetWindowUserPointer(window));
        if (app) {
            // Unikanie zoomowania, jeśli mysz jest nad oknem ImGui
            if (!ImGui::GetIO().WantCaptureMouse) {
                float zoomFactor = (yoffset > 0) ? 1.1f : 1.0f / 1.1f;
                app->zoomLevel *= zoomFactor;
                app->recalculateRange();
            }
        }
        ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
    }

    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
        FunctionVisualizer* app = static_cast<FunctionVisualizer*>(glfwGetWindowUserPointer(window));
        if (app && app->isPanning && !ImGui::GetIO().WantCaptureMouse) {
            double deltaX = xpos - app->lastMouseX;
            double deltaY = ypos - app->lastMouseY;

            int width, height;
            glfwGetWindowSize(window, &width, &height);

            // Przeliczenie delty pikselowej na deltę w jednostkach wykresu
            float plotWidth = app->renderer.getXMax() - app->renderer.getXMin();
            float plotHeight = app->renderer.getYMax() - app->renderer.getYMin();
            
            app->panX -= (float)deltaX * plotWidth / (float)width;
            app->panY += (float)deltaY * plotHeight / (float)height; // Odwrócenie Y

            app->lastMouseX = xpos;
            app->lastMouseY = ypos;
            app->recalculateRange();
        }
        ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
    }

    void setupCallbacks() {
        glfwSetWindowUserPointer(window, this);
        glfwSetMouseButtonCallback(window, mouseButtonCallback);
        glfwSetScrollCallback(window, scrollCallback);
        glfwSetCursorPosCallback(window, cursorPosCallback);
    }

    // --- Logika wykresu ---
    void recalculateRange() {
        float defaultRange = 10.0f;
        float xRange = defaultRange / zoomLevel;
        float xMin = -xRange + panX;
        float xMax = xRange + panX;
        renderer.setXRange(xMin, xMax);
        generator.setRange(xMin, xMax);
    }
    
    std::vector<std::vector<Point>> generateAllPlots() {
        std::vector<std::vector<Point>> plots;
        for (const auto& func : functionList) {
            plots.push_back(generator.generatePlot(func));
        }
        return plots;
    }

    // --- Renderowanie ---
    void render() {
        // Obliczanie nowego zakresu na podstawie zoom/pan
        recalculateRange();
        
        // Generowanie punktów dla wszystkich widocznych funkcji
        std::vector<std::vector<Point>> plots = generateAllPlots();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Rysowanie wykresów
        renderer.drawPlots(plots, functionList);
        
        // Renderowanie ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        renderImGui();
        
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(window);
    }
    
    void renderImGui() {
        // --- Panel Kontrolny ---
        ImGui::Begin("Kontroler Wykresów 📈");
        
        // --- Dodawanie nowej funkcji ---
        ImGui::Text("Dodaj nową funkcję:");
        static int newFunctionType = (int)FunctionType::Linear;
        ImGui::Combo("Typ", &newFunctionType, functionNames, IM_ARRAYSIZE(functionNames));
        ImGui::SameLine();
        if (ImGui::Button("➕ Dodaj")) {
            FunctionParams newFunc;
            newFunc.type = (FunctionType)newFunctionType;
            newFunc.label = "Wykres " + std::to_string(functionList.size() + 1);
            // Wygenerowanie losowego koloru
            newFunc.color = ImVec4((float)rand() / RAND_MAX, (float)rand() / RAND_MAX, (float)rand() / RAND_MAX, 1.0f);
            functionList.push_back(newFunc);
        }
        ImGui::Separator();
        
        // --- Lista funkcji ---
        ImGui::Text("Aktywne wykresy: %zu", functionList.size());
        ImGui::Text("Zoom: %.2f | Pan X: %.2f | Pan Y: %.2f", zoomLevel, panX, panY);

        ImGui::SeparatorText("Interakcja z wykresem");
        if (ImGui::Button("Resetuj Zoom/Pan")) {
            zoomLevel = 1.0f;
            panX = 0.0f;
            panY = 0.0f;
        }

        ImGui::SeparatorText("Ustawienia funkcji");
        
        // --- Edycja istniejących funkcji ---
        int indexToDelete = -1;
        for (size_t i = 0; i < functionList.size(); ++i) {
            FunctionParams& func = functionList[i];
            
            // Unikalny ID dla każdego elementu
            ImGui::PushID((int)i);

            // Używamy Tree Node do zwijania/rozwijania
            if (ImGui::TreeNode(func.label.c_str())) {
                
                ImGui::Checkbox("Widoczny", &func.isVisible);

                // Zmiana typu
                int currentType = (int)func.type;
                if (ImGui::Combo("Typ funkcji", &currentType, functionNames, IM_ARRAYSIZE(functionNames))) {
                    func.type = (FunctionType)currentType;
                }
                
                // Edycja parametrów za pomocą sliderów
                switch (func.type) {
                    case FunctionType::Linear:
                        ImGui::SliderFloat("a", &func.p1, -5.0f, 5.0f);
                        ImGui::SliderFloat("c", &func.p3, -10.0f, 10.0f);
                        break;
                    case FunctionType::Quadratic:
                        ImGui::SliderFloat("a (x²)", &func.p1, -5.0f, 5.0f);
                        ImGui::SliderFloat("b (x)", &func.p2, -5.0f, 5.0f);
                        ImGui::SliderFloat("c (stała)", &func.p3, -10.0f, 10.0f);
                        break;
                    case FunctionType::Sinus:
                        ImGui::SliderFloat("Amplituda (a)", &func.p1, -5.0f, 5.0f);
                        ImGui::SliderFloat("Częstotliwość (b)", &func.p2, 0.1f, 5.0f);
                        ImGui::SliderFloat("Przesunięcie fazowe (c)", &func.p3, (float)-M_PI, (float)M_PI);
                        break;
                    case FunctionType::Exponential:
                        ImGui::SliderFloat("Współczynnik (a)", &func.p1, -5.0f, 5.0f);
                        ImGui::SliderFloat("Wykładnik (b)", &func.p2, -1.0f, 1.0f);
                        break;
                    case FunctionType::Logarithmic:
                        ImGui::SliderFloat("Współczynnik (a)", &func.p1, -5.0f, 5.0f);
                        ImGui::SliderFloat("Mnożnik (b)", &func.p2, 0.1f, 5.0f);
                        ImGui::SliderFloat("Przesunięcie (c)", &func.p3, -5.0f, 5.0f);
                        break;
                    case FunctionType::Tangent:
                        ImGui::SliderFloat("Amplituda (a)", &func.p1, -5.0f, 5.0f);
                        ImGui::SliderFloat("Częstotliwość (b)", &func.p2, 0.1f, 5.0f);
                        ImGui::SliderFloat("Przesunięcie fazowe (c)", &func.p3, (float)-M_PI, (float)M_PI);
                        break;
                    case FunctionType::SquareRoot:
                        ImGui::SliderFloat("Współczynnik (a)", &func.p1, -5.0f, 5.0f);
                        ImGui::SliderFloat("Mnożnik (b)", &func.p2, 0.1f, 5.0f);
                        ImGui::SliderFloat("Przesunięcie (c)", &func.p3, -5.0f, 5.0f);
                        break;
                }
                
                ImGui::ColorEdit3("Kolor", (float*)&func.color);
                
                ImGui::SameLine();
                if (ImGui::Button("Usuń")) {
                    indexToDelete = (int)i;
                }
                
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
        
        // Obsługa usuwania po pętli
        if (indexToDelete != -1) {
            functionList.erase(functionList.begin() + indexToDelete);
        }

        // --- Informacje o zakresie ---
        ImGui::SeparatorText("Aktualny Zakres");
        ImGui::Text("X: [%.2f, %.2f]", renderer.getXMin(), renderer.getXMax());
        ImGui::Text("Y: [%.2f, %.2f] (Auto-Skalowanie)", renderer.getYMin(), renderer.getYMax());
        
        ImGui::End(); // Koniec Panelu Kontrolnego
    }
};

// --- Główna funkcja programu ---
int main() {
    // Ustawienie ziarna losowości dla różnych kolorów wykresów
    srand(time(0));
    
    FunctionVisualizer app;
    
    if (app.init()) {
        app.run();
    }
    
    app.cleanup();
    return 0;
}
