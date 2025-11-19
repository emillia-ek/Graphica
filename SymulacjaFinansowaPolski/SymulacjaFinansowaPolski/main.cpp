#include <iostream>
#include <cmath>
#include <vector>
#include <string>

// OpenGL i GLFW headers
#include <GLFW/glfw3.h>
#include "imconfig.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// Struktura przechowująca punkt na wykresie
struct Point {
    float x, y;
    Point(float x, float y) : x(x), y(y) {}
};

// Klasa zarządzająca wykresem
class FunctionPlotter {
private:
    std::vector<Point> points;
    float xMin, xMax;
    int resolution;
    
public:
    FunctionPlotter() : xMin(-10.0f), xMax(10.0f), resolution(200) {}
    
    // Czyszczenie punktów
    void clear() {
        points.clear();
    }
    
    // Generowanie punktów dla funkcji liniowej: y = ax + b
    void generateLinear(float a, float b) {
        clear();
        float step = (xMax - xMin) / resolution;
        for (int i = 0; i <= resolution; ++i) {
            float x = xMin + i * step;
            float y = a * x + b;
            points.emplace_back(x, y);
        }
    }
    
    // Generowanie punktów dla funkcji kwadratowej: y = ax² + bx + c
    void generateQuadratic(float a, float b, float c) {
        clear();
        float step = (xMax - xMin) / resolution;
        for (int i = 0; i <= resolution; ++i) {
            float x = xMin + i * step;
            float y = a * x * x + b * x + c;
            points.emplace_back(x, y);
        }
    }
    
    // Generowanie punktów dla funkcji sinus: y = a*sin(bx + c)
    void generateSin(float a, float b, float c) {
        clear();
        float step = (xMax - xMin) / resolution;
        for (int i = 0; i <= resolution; ++i) {
            float x = xMin + i * step;
            float y = a * sin(b * x + c);
            points.emplace_back(x, y);
        }
    }
    
    // Generowanie punktów dla funkcji wykładniczej: y = a * exp(bx)
    void generateExponential(float a, float b) {
        clear();
        float step = (xMax - xMin) / resolution;
        for (int i = 0; i <= resolution; ++i) {
            float x = xMin + i * step;
            float y = a * exp(b * x);
            points.emplace_back(x, y);
        }
    }
    
    // Generowanie punktów dla funkcji logarytmicznej: y = a * log(bx + c)
    void generateLogarithmic(float a, float b, float c) {
        clear();
        float step = (xMax - xMin) / resolution;
        for (int i = 0; i <= resolution; ++i) {
            float x = xMin + i * step;
            float arg = b * x + c;
            if (arg > 0) {
                float y = a * log(arg);
                points.emplace_back(x, y);
            }
        }
    }
    
    // Rysowanie wykresu
    void draw() {
        if (points.empty()) return;
        
        glBegin(GL_LINE_STRIP);
        glColor3f(1.0f, 0.5f, 0.2f); // Pomarańczowy kolor
        
        for (const auto& point : points) {
            glVertex2f(point.x, point.y);
        }
        
        glEnd();
    }
    
    // Ustawienia zakresu
    void setRange(float min, float max) {
        xMin = min;
        xMax = max;
    }
    
    float getXMin() const { return xMin; }
    float getXMax() const { return xMax; }
};

// Klasa zarządzająca siatką i osiami
class CoordinateSystem {
private:
    float xMin, xMax, yMin, yMax;
    
public:
    CoordinateSystem() : xMin(-10.0f), xMax(10.0f), yMin(-10.0f), yMax(10.0f) {}
    
    void draw() {
        // Rysowanie siatki
        glColor3f(0.3f, 0.3f, 0.3f);
        glBegin(GL_LINES);
        
        // Linie poziome
        for (float y = yMin; y <= yMax; y += 1.0f) {
            if (fabs(y) > 0.01f) { // Pomijamy oś X
                glVertex2f(xMin, y);
                glVertex2f(xMax, y);
            }
        }
        
        // Linie pionowe
        for (float x = xMin; x <= xMax; x += 1.0f) {
            if (fabs(x) > 0.01f) { // Pomijamy oś Y
                glVertex2f(x, yMin);
                glVertex2f(x, yMax);
            }
        }
        
        glEnd();
        
        // Rysowanie osi
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_LINES);
        
        // Oś X
        glVertex2f(xMin, 0.0f);
        glVertex2f(xMax, 0.0f);
        
        // Oś Y
        glVertex2f(0.0f, yMin);
        glVertex2f(0.0f, yMax);
        
        glEnd();
        
        // Strzałki na osiach
        glBegin(GL_TRIANGLES);
        
        // Strzałka na osi X
        glVertex2f(xMax, 0.0f);
        glVertex2f(xMax - 0.3f, 0.1f);
        glVertex2f(xMax - 0.3f, -0.1f);
        
        // Strzałka na osi Y
        glVertex2f(0.0f, yMax);
        glVertex2f(0.1f, yMax - 0.3f);
        glVertex2f(-0.1f, yMax - 0.3f);
        
        glEnd();
    }
    
    void setRange(float xmin, float xmax, float ymin, float ymax) {
        xMin = xmin;
        xMax = xmax;
        yMin = ymin;
        yMax = ymax;
    }
};

// Globalne zmienne
GLFWwindow* window;
FunctionPlotter plotter;
CoordinateSystem coordSystem;
int currentFunction = 0;

// Parametry funkcji
float linearA = 1.0f, linearB = 0.0f;
float quadraticA = 1.0f, quadraticB = 0.0f, quadraticC = 0.0f;
float sinA = 1.0f, sinB = 1.0f, sinC = 0.0f;
float expA = 1.0f, expB = 0.1f;
float logA = 1.0f, logB = 1.0f, logC = 1.0f;
float rangeMin = -10.0f, rangeMax = 10.0f;

// Funkcja inicjalizująca GLFW
bool initGLFW() {
    if (!glfwInit()) {
        std::cerr << "Błąd inicjalizacji GLFW" << std::endl;
        return false;
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    
    window = glfwCreateWindow(1200, 800, "Wizualizator Funkcji Matematycznych", NULL, NULL);
    if (!window) {
        std::cerr << "Błąd tworzenia okna GLFW" << std::endl;
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // V-Sync
    
    return true;
}

// Funkcja inicjalizująca ImGui
void initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    ImGui::StyleColorsDark();
    
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 120");
}

// Funkcja renderująca
void render() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Ustawienie macierzy projekcji
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(rangeMin, rangeMax, rangeMin, rangeMax, -1.0f, 1.0f);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Rysowanie układu współrzędnych
    coordSystem.setRange(rangeMin, rangeMax, rangeMin, rangeMax);
    coordSystem.draw();
    
    // Rysowanie wykresu funkcji
    plotter.draw();
    
    // Renderowanie ImGui
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    // Panel kontrolny ImGui
    ImGui::Begin("Kontroler Funkcji");
    
    ImGui::Text("Wybierz typ funkcji:");
    const char* functions[] = { "Liniowa", "Kwadratowa", "Sinus", "Wykładnicza", "Logarytmiczna" };
    ImGui::Combo("Funkcja", &currentFunction, functions, IM_ARRAYSIZE(functions));
    
    ImGui::Separator();
    ImGui::Text("Parametry funkcji:");
    
    switch (currentFunction) {
        case 0: // Liniowa
            ImGui::InputFloat("a (współczynnik)", &linearA);
            ImGui::InputFloat("b (wyraz wolny)", &linearB);
            if (ImGui::Button("Rysuj funkcję liniową")) {
                plotter.setRange(rangeMin, rangeMax);
                plotter.generateLinear(linearA, linearB);
            }
            break;
            
        case 1: // Kwadratowa
            ImGui::InputFloat("a (x²)", &quadraticA);
            ImGui::InputFloat("b (x)", &quadraticB);
            ImGui::InputFloat("c (stała)", &quadraticC);
            if (ImGui::Button("Rysuj funkcję kwadratową")) {
                plotter.setRange(rangeMin, rangeMax);
                plotter.generateQuadratic(quadraticA, quadraticB, quadraticC);
            }
            break;
            
        case 2: // Sinus
            ImGui::InputFloat("Amplituda (a)", &sinA);
            ImGui::InputFloat("Częstotliwość (b)", &sinB);
            ImGui::InputFloat("Przesunięcie fazowe (c)", &sinC);
            if (ImGui::Button("Rysuj funkcję sinus")) {
                plotter.setRange(rangeMin, rangeMax);
                plotter.generateSin(sinA, sinB, sinC);
            }
            break;
            
        case 3: // Wykładnicza
            ImGui::InputFloat("Współczynnik (a)", &expA);
            ImGui::InputFloat("Wykładnik (b)", &expB);
            if (ImGui::Button("Rysuj funkcję wykładniczą")) {
                plotter.setRange(rangeMin, rangeMax);
                plotter.generateExponential(expA, expB);
            }
            break;
            
        case 4: // Logarytmiczna
            ImGui::InputFloat("Współczynnik (a)", &logA);
            ImGui::InputFloat("Mnożnik argumentu (b)", &logB);
            ImGui::InputFloat("Przesunięcie (c)", &logC);
            if (ImGui::Button("Rysuj funkcję logarytmiczną")) {
                plotter.setRange(rangeMin, rangeMax);
                plotter.generateLogarithmic(logA, logB, logC);
            }
            break;
    }
    
    ImGui::Separator();
    ImGui::Text("Zakres wykresu:");
    ImGui::InputFloat("X min", &rangeMin);
    ImGui::InputFloat("X max", &rangeMax);
    
    if (ImGui::Button("Resetuj zakres")) {
        rangeMin = -10.0f;
        rangeMax = 10.0f;
    }
    
    ImGui::Separator();
    if (ImGui::Button("Wyczyść wykres")) {
        plotter.clear();
    }
    
    ImGui::End();
    
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
    glfwSwapBuffers(window);
}

// Funkcja czyszcząca zasoby
void cleanup() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    glfwDestroyWindow(window);
    glfwTerminate();
}

// Główna pętla programu
int main() {
    if (!initGLFW()) {
        return -1;
    }
    
    initImGui();
    
    // Główna pętla
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        render();
    }
    
    cleanup();
    return 0;
}
