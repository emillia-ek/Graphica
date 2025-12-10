#include <iostream>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>
#include <sstream>

// OpenGL i GLFW headers
#include <GLFW/glfw3.h>
#include "imconfig.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// --- Deklaracje struktur i wyliczeń ---

enum class FunctionType {
    Linear,
    Quadratic,
    Sinus,
    Exponential,
    Logarithmic,
    SquareRoot
};

struct FunctionParams {
    float p1 = 1.0f;
    float p2 = 1.0f;
    float p3 = 0.0f;
    ImVec4 color = ImVec4(1.0f, 0.5f, 0.2f, 1.0f);
    FunctionType type = FunctionType::Linear;
    std::string label = "y = x";
    bool isVisible = true;
    
    // Metoda do generowania równania na podstawie parametrów
    std::string generateEquation() const {
        std::stringstream ss;
        ss << "y = ";
        
        switch (type) {
            case FunctionType::Linear:
                if (p1 != 0) {
                    if (p1 == 1.0f) ss << "x";
                    else if (p1 == -1.0f) ss << "-x";
                    else ss << std::fixed << std::setprecision(2) << p1 << "x";
                    
                    if (p3 > 0) ss << " + " << std::fixed << std::setprecision(2) << p3;
                    else if (p3 < 0) ss << " - " << std::fixed << std::setprecision(2) << std::abs(p3);
                } else {
                    ss << std::fixed << std::setprecision(2) << p3;
                }
                break;
                
            case FunctionType::Quadratic:
                if (p1 != 0) {
                    if (p1 == 1.0f) ss << "x²";
                    else if (p1 == -1.0f) ss << "-x²";
                    else ss << std::fixed << std::setprecision(2) << p1 << "x²";
                    
                    if (p2 > 0) ss << " + " << std::fixed << std::setprecision(2) << p2 << "x";
                    else if (p2 < 0) ss << " - " << std::fixed << std::setprecision(2) << std::abs(p2) << "x";
                    
                    if (p3 > 0) ss << " + " << std::fixed << std::setprecision(2) << p3;
                    else if (p3 < 0) ss << " - " << std::fixed << std::setprecision(2) << std::abs(p3);
                } else if (p2 != 0) {
                    if (p2 == 1.0f) ss << "x";
                    else if (p2 == -1.0f) ss << "-x";
                    else ss << std::fixed << std::setprecision(2) << p2 << "x";
                    
                    if (p3 > 0) ss << " + " << std::fixed << std::setprecision(2) << p3;
                    else if (p3 < 0) ss << " - " << std::fixed << std::setprecision(2) << std::abs(p3);
                } else {
                    ss << std::fixed << std::setprecision(2) << p3;
                }
                break;
                
            case FunctionType::Sinus:
                if (p1 != 1.0f) {
                    if (p1 == -1.0f) ss << "-";
                    else ss << std::fixed << std::setprecision(2) << p1;
                }
                ss << "sin(";
                if (p2 != 1.0f) ss << std::fixed << std::setprecision(2) << p2 << "x";
                else ss << "x";
                
                if (p3 > 0) ss << " + " << std::fixed << std::setprecision(2) << p3;
                else if (p3 < 0) ss << " - " << std::fixed << std::setprecision(2) << std::abs(p3);
                ss << ")";
                break;
                
            case FunctionType::Exponential:
                if (p1 != 1.0f) {
                    if (p1 == -1.0f) ss << "-";
                    else ss << std::fixed << std::setprecision(2) << p1;
                }
                ss << "exp(";
                if (p2 != 1.0f) ss << std::fixed << std::setprecision(2) << p2 << "x";
                else ss << "x";
                ss << ")";
                break;
                
            case FunctionType::Logarithmic:
                if (p1 != 1.0f) {
                    if (p1 == -1.0f) ss << "-";
                    else ss << std::fixed << std::setprecision(2) << p1;
                }
                ss << "ln(";
                if (p2 != 1.0f) ss << std::fixed << std::setprecision(2) << p2 << "x";
                else ss << "x";
                
                if (p3 > 0) ss << " + " << std::fixed << std::setprecision(2) << p3;
                else if (p3 < 0) ss << " - " << std::fixed << std::setprecision(2) << std::abs(p3);
                ss << ")";
                break;
                
            case FunctionType::SquareRoot:
                if (p1 != 1.0f) {
                    if (p1 == -1.0f) ss << "-";
                    else ss << std::fixed << std::setprecision(2) << p1;
                }
                ss << "√(";
                if (p2 != 1.0f) ss << std::fixed << std::setprecision(2) << p2 << "x";
                else ss << "x";
                
                if (p3 > 0) ss << " + " << std::fixed << std::setprecision(2) << p3;
                else if (p3 < 0) ss << " - " << std::fixed << std::setprecision(2) << std::abs(p3);
                ss << ")";
                break;
        }
        
        return ss.str();
    }
};

struct Point {
    float x, y;
    Point(float x, float y) : x(x), y(y) {}
};

// --- Klasa zarządzająca wykresem ---

class PlotGenerator {
private:
    float xMin, xMax;
    int resolution = 500;

public:
    PlotGenerator() : xMin(-10.0f), xMax(10.0f) {}

    void setRange(float min, float max) {
        xMin = min;
        xMax = max;
    }

    float calculateY(FunctionType type, float x, const FunctionParams& params) {
        switch (type) {
            case FunctionType::Linear:
                return params.p1 * x + params.p3;
            case FunctionType::Quadratic:
                return params.p1 * x * x + params.p2 * x + params.p3;
            case FunctionType::Sinus:
                return params.p1 * std::sin(params.p2 * x + params.p3);
            case FunctionType::Exponential:
                return params.p1 * std::exp(params.p2 * x);
            case FunctionType::Logarithmic:
                if (params.p2 * x + params.p3 > 0)
                    return params.p1 * std::log(params.p2 * x + params.p3);
                return NAN;
            case FunctionType::SquareRoot:
                if (params.p2 * x + params.p3 >= 0)
                    return params.p1 * std::sqrt(params.p2 * x + params.p3);
                return NAN;
        }
        return 0.0f;
    }

    std::vector<Point> generatePlot(const FunctionParams& params) {
        std::vector<Point> points;
        float step = (xMax - xMin) / resolution;
        
        for (int i = 0; i <= resolution; ++i) {
            float x = xMin + i * step;
            float y = calculateY(params.type, x, params);
            
            bool is_valid = !std::isnan(y) && std::isfinite(y) && std::abs(y) < 1e6;

            if (is_valid) {
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
    const float FIXED_GRID_STEP = 1.0f;

public:
    PlotRenderer() : xMin(-10.0f), xMax(10.0f), yMin(-10.0f), yMax(10.0f) {}

    void drawPlots(const std::vector<std::vector<Point>>& plots, const std::vector<FunctionParams>& paramsList) {
        
        // Zakres Y jest taki sam jak X, aby siatka była proporcjonalna
        yMin = xMin;
        yMax = xMax;

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(xMin, xMax, yMin, yMax, -1.0f, 1.0f);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        drawCoordinateSystem();

        for (size_t i = 0; i < plots.size(); ++i) {
            if (paramsList[i].isVisible) {
                drawSinglePlot(plots[i], paramsList[i].color);
            }
        }
    }

    void drawSinglePlot(const std::vector<Point>& points, const ImVec4& color) {
        if (points.empty()) return;

        glColor3f(color.x, color.y, color.z);
        glLineWidth(2.0f);

        glBegin(GL_LINES);
        for (size_t i = 1; i < points.size(); ++i) {
            glVertex2f(points[i-1].x, points[i-1].y);
            glVertex2f(points[i].x, points[i].y);
        }
        glEnd();
        glLineWidth(1.0f);
    }

    void drawCoordinateSystem() {
        // --- Siatka ---
        glColor3f(0.2f, 0.2f, 0.2f);
        glBegin(GL_LINES);

        float step = FIXED_GRID_STEP;

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
        
        // --- Rysowanie osi ---
        glColor3f(0.8f, 0.8f, 0.8f);
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
    }

    void setXRange(float min, float max) { xMin = min; xMax = max; }
    float getXMin() const { return xMin; }
    float getXMax() const { return xMax; }
    float getYMin() const { return xMin; }
    float getYMax() const { return xMax; }
    float getGridStep() const { return FIXED_GRID_STEP; }
};

// --- Główna klasa aplikacji (kontroler) ---

class FunctionVisualizer {
private:
    GLFWwindow* window = nullptr;
    PlotGenerator generator;
    PlotRenderer renderer;
    std::vector<FunctionParams> functionList;
    const char* functionNames[6] = { "Liniowa (a*x + c)", "Kwadratowa (a*x² + b*x + c)", "Sinus (a*sin(b*x + c))",
                                     "Wykładnicza (a*exp(b*x))", "Logarytmiczna (a*ln(b*x + c))",
                                     "Pierwiastek (a*sqrt(b*x + c))" };
    
    float zoomLevel = 1.0f;
    float panX = 0.0f, panY = 0.0f;
    double lastMouseX, lastMouseY;
    bool isPanning = false;
    
    int editingIndex = -1;  // Indeks funkcji w trakcie edycji (-1 oznacza brak edycji)
    
    // Buforowane wartości do edycji
    float editP1 = 0.0f;
    float editP2 = 0.0f;
    float editP3 = 0.0f;
    ImVec4 editColor;
    int editType = 0;

    char newLabelBuffer[128] = ""; // Nie używamy już tego dla nowych funkcji

public:
    FunctionVisualizer() {
        functionList.emplace_back(FunctionParams{1.0f, 0.0f, 0.0f, ImVec4(1.0f, 0.5f, 0.2f, 1.0f), FunctionType::Linear});
        // Automatycznie generujemy równanie dla domyślnej funkcji
        functionList.back().label = functionList.back().generateEquation();
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
    // --- Obsługa zdarzeń ---
    bool initGLFW() {
        if (!glfwInit()) {
            std::cerr << "Błąd inicjalizacji GLFW" << std::endl;
            return false;
        }
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        window = glfwCreateWindow(1200, 800, "Wizualizator Funkcji", NULL, NULL);
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
    
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
        FunctionVisualizer* app = static_cast<FunctionVisualizer*>(glfwGetWindowUserPointer(window));
        if (app && button == GLFW_MOUSE_BUTTON_LEFT) {
            bool isMouseOverImGui = ImGui::GetIO().WantCaptureMouse;
            if (action == GLFW_PRESS && !isMouseOverImGui) {
                app->isPanning = true;
                glfwGetCursorPos(window, &app->lastMouseX, &app->lastMouseY);
            } else if (action == GLFW_RELEASE) {
                app->isPanning = false;
            }
        }
        ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    }

    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
        FunctionVisualizer* app = static_cast<FunctionVisualizer*>(glfwGetWindowUserPointer(window));
        if (app) {
            if (!ImGui::GetIO().WantCaptureMouse) {
                float zoomFactor = (yoffset > 0) ? 1.2f : 1.0f / 1.2f;
                app->zoomLevel *= zoomFactor;
                app->recalculateRange();
            }
        }
        ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
    }

    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
        FunctionVisualizer* app = static_cast<FunctionVisualizer*>(glfwGetWindowUserPointer(window));
        if (app && app->isPanning) {
            double deltaX = xpos - app->lastMouseX;
            double deltaY = ypos - app->lastMouseY;

            int width, height;
            glfwGetWindowSize(window, &width, &height);
            
            float plotWidth = app->renderer.getXMax() - app->renderer.getXMin();
            
            app->panX -= (float)deltaX * (plotWidth / (float)width);
            app->panY += (float)deltaY * (plotWidth / (float)height);

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

    void recalculateRange() {
        zoomLevel = std::max(0.01f, zoomLevel);
        
        float defaultHalfRange = 10.0f;
        float currentHalfRange = defaultHalfRange / zoomLevel;
        
        float xMin = -currentHalfRange + panX;
        float xMax = currentHalfRange + panX;
        
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

    void render() {
        recalculateRange();
        std::vector<std::vector<Point>> plots = generateAllPlots();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        renderer.drawPlots(plots, functionList);
        
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
        ImGui::Begin("Kontroler Wykresów \U0001F4C8");
        
        // --- Dodawanie nowej funkcji ---
        ImGui::Text("Dodaj nową funkcję:");
        static int newFunctionType = (int)FunctionType::Linear;
        
        ImGui::Combo("Typ", &newFunctionType, functionNames, IM_ARRAYSIZE(functionNames));
        
        if (ImGui::Button("\u2795 Dodaj")) {
            FunctionParams newFunc;
            newFunc.type = (FunctionType)newFunctionType;
            newFunc.color = ImVec4((float)rand() / RAND_MAX, (float)rand() / RAND_MAX, (float)rand() / RAND_MAX, 1.0f);
            
            // Ustawienie domyślnych bezpiecznych wartości
            if (newFunc.type == FunctionType::Logarithmic || newFunc.type == FunctionType::SquareRoot || newFunc.type == FunctionType::Sinus) {
                newFunc.p2 = 1.0f;
            } else if (newFunc.type == FunctionType::Quadratic) {
                newFunc.p2 = 0.0f;
            }
            
            // Generowanie równania jako nazwy
            newFunc.label = newFunc.generateEquation();
            functionList.push_back(newFunc);
        }
        
        ImGui::Separator();
        
        // --- Informacje o interakcji ---
        ImGui::SeparatorText("Interakcja z wykresem");
        ImGui::Text("Zoom: %.2fx | Pan: (%.2f, %.2f)", zoomLevel, panX, panY);
        ImGui::Text("Krok siatki: %.1f (stały)", renderer.getGridStep());

        if (ImGui::Button("Resetuj Zoom/Pan")) {
            zoomLevel = 1.0f;
            panX = 0.0f;
            panY = 0.0f;
        }

        ImGui::SeparatorText("Lista i ustawienia funkcji");
        
        // --- Edycja istniejących funkcji ---
        int indexToDelete = -1;
        for (size_t i = 0; i < functionList.size(); ++i) {
            FunctionParams& func = functionList[i];
            ImGui::PushID((int)i);

            // Wyświetlenie
            ImGui::ColorButton("##Color", func.color, ImGuiColorEditFlags_NoTooltip);
            ImGui::SameLine();
            ImGui::Checkbox("##Visible", &func.isVisible);
            ImGui::SameLine();
            
            // Wyświetlanie równania jako nazwy
            ImGui::Text("%s", func.label.c_str());
            
            ImGui::SameLine(ImGui::GetWindowWidth() - 120.0f);

            // Przycisk "Usuń"
            if (ImGui::Button("Usuń \u274C")) {
                indexToDelete = (int)i;
            }
            
            ImGui::SameLine();

            // Przycisk "Edytuj"
            std::string edit_id = "Edytuj \u270F\uFE0F##" + std::to_string(i);
            if (ImGui::Button(edit_id.c_str())) {
                editingIndex = (int)i;
                // Zapisanie oryginalnych wartości do buforów edycji
                editP1 = func.p1;
                editP2 = func.p2;
                editP3 = func.p3;
                editColor = func.color;
                editType = (int)func.type;
            }
            ImGui::PopID();
            ImGui::Separator();
        }
        
        // --- Modalne okno edycji ---
        if (editingIndex >= 0 && editingIndex < (int)functionList.size()) {
            ImGui::OpenPopup("Edycja Funkcji");
        }
        
        if (ImGui::BeginPopupModal("Edycja Funkcji", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Edycja funkcji:");
            
            // Wyświetlenie aktualnego równania
            FunctionParams tempFunc;
            tempFunc.p1 = editP1;
            tempFunc.p2 = editP2;
            tempFunc.p3 = editP3;
            tempFunc.type = (FunctionType)editType;
            std::string currentEquation = tempFunc.generateEquation();
            ImGui::Text("Równanie: %s", currentEquation.c_str());
            
            ImGui::Separator();

            // 1. Kolor
            ImGui::Text("Kolor:");
            ImGui::ColorEdit3("##Kolor", (float*)&editColor);
            ImGui::Separator();

            // 2. Typ funkcji
            ImGui::Text("Typ funkcji:");
            if (ImGui::Combo("##Typ", &editType, functionNames, IM_ARRAYSIZE(functionNames))) {
                // Resetowanie p2 przy zmianie typu
                FunctionType newType = (FunctionType)editType;
                if (newType == FunctionType::Logarithmic || newType == FunctionType::SquareRoot || newType == FunctionType::Sinus) {
                    editP2 = std::max(0.01f, editP2);
                } else if (newType == FunctionType::Quadratic) {
                    editP2 = 0.0f;
                } else {
                    editP2 = 1.0f;
                }
            }
            ImGui::Separator();
            
            // 3. Edycja parametrów (p1, p2, p3) - TYLKO INPUT NUMERYCZNY
            ImGui::Text("Parametry funkcji:");
            
            // Etykiety parametrów w zależności od typu funkcji
            std::string p1_label, p2_label, p3_label;
            switch ((FunctionType)editType) {
                case FunctionType::Linear:
                    p1_label = "a (współczynnik):";
                    p3_label = "c (wyraz wolny):";
                    break;
                case FunctionType::Quadratic:
                    p1_label = "a (x²):";
                    p2_label = "b (x):";
                    p3_label = "c (stała):";
                    break;
                case FunctionType::Sinus:
                    p1_label = "a (amplituda):";
                    p2_label = "b (częstotliwość):";
                    p3_label = "c (faza):";
                    break;
                case FunctionType::Exponential:
                    p1_label = "a (współczynnik):";
                    p2_label = "b (wykładnik):";
                    break;
                case FunctionType::Logarithmic:
                    p1_label = "a (współczynnik):";
                    p2_label = "b (mnożnik):";
                    p3_label = "c (przesunięcie):";
                    break;
                case FunctionType::SquareRoot:
                    p1_label = "a (współczynnik):";
                    p2_label = "b (mnożnik):";
                    p3_label = "c (przesunięcie):";
                    break;
            }
            
            ImGui::Text("%s", p1_label.c_str());
            ImGui::SameLine(150);
            ImGui::PushItemWidth(200);
            ImGui::InputFloat("##P1", &editP1, 0.1f, 1.0f, "%.3f");
            ImGui::PopItemWidth();
            
            if (!p2_label.empty()) {
                ImGui::Text("%s", p2_label.c_str());
                ImGui::SameLine(150);
                ImGui::PushItemWidth(200);
                ImGui::InputFloat("##P2", &editP2, 0.1f, 1.0f, "%.3f");
                ImGui::PopItemWidth();
            }
            
            if (!p3_label.empty()) {
                ImGui::Text("%s", p3_label.c_str());
                ImGui::SameLine(150);
                ImGui::PushItemWidth(200);
                ImGui::InputFloat("##P3", &editP3, 0.1f, 1.0f, "%.3f");
                ImGui::PopItemWidth();
            }
            
            // Walidacja parametrów w zależności od typu funkcji
            bool validParams = true;
            std::string errorMessage = "";
            
            if ((FunctionType)editType == FunctionType::Logarithmic ||
                (FunctionType)editType == FunctionType::SquareRoot) {
                if (editP2 <= 0.0f) {
                    validParams = false;
                    errorMessage = "P2 (b) musi być większe od 0 dla logarytmu i pierwiastka!";
                }
            } else if ((FunctionType)editType == FunctionType::Sinus) {
                if (editP2 <= 0.0f) {
                    validParams = false;
                    errorMessage = "P2 (b) musi być większe od 0 dla funkcji sinus!";
                }
            }
            
            if (!validParams) {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", errorMessage.c_str());
            }
            
            ImGui::Separator();
            
            // Przyciski akcji
            float buttonWidth = 120.0f;
            float space = (ImGui::GetContentRegionAvail().x - buttonWidth * 2) / 3.0f;
            
            ImGui::Dummy(ImVec2(space, 0));
            ImGui::SameLine();
            
            if (ImGui::Button("Zapisz", ImVec2(buttonWidth, 0)) && validParams) {
                // Zastosowanie zmian i wygenerowanie nowego równania
                FunctionParams& func = functionList[editingIndex];
                func.p1 = editP1;
                func.p2 = editP2;
                func.p3 = editP3;
                func.color = editColor;
                func.type = (FunctionType)editType;
                // Automatyczne wygenerowanie nowego równania
                func.label = func.generateEquation();
                editingIndex = -1;
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(space, 0));
            ImGui::SameLine();
            
            if (ImGui::Button("Anuluj", ImVec2(buttonWidth, 0))) {
                // Odrzucenie zmian - przywrócenie oryginalnych wartości
                editingIndex = -1;
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }

        // Obsługa usuwania po pętli
        if (indexToDelete != -1) {
            if (editingIndex == indexToDelete) {
                editingIndex = -1;
            } else if (editingIndex > indexToDelete) {
                editingIndex--;
            }
            functionList.erase(functionList.begin() + indexToDelete);
        }

        ImGui::SeparatorText("Aktualny Zakres Wykresu");
        ImGui::Text("X: [%.2f, %.2f]", renderer.getXMin(), renderer.getXMax());
        ImGui::Text("Y: [%.2f, %.2f] (Stała siatka)", renderer.getYMin(), renderer.getYMax());
        
        ImGui::End();
    }
};

// --- Główna funkcja programu ---
int main() {
    srand(static_cast<unsigned int>(time(0)));
    
    FunctionVisualizer app;
    
    if (app.init()) {
        app.run();
    }
    
    app.cleanup();
    return 0;
}
