#include "Application.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <string>
#include <cstring>
#include <cctype>
#include <algorithm>
#include <memory>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "MathExpressionParser.h" // Upewnij się, że ten plik jest załączony

using namespace std;

// Static instance pointer for callbacks
static Application* g_ApplicationInstance = nullptr;

Application::Application() : window(nullptr), rangeMin(-10.0f), rangeMax(10.0f),
                            showHelp(false), isDragging(false), lastMouseX(0), lastMouseY(0) {
    strcpy(equationInput, "y=x");
}

Application::~Application() {
    cleanup();
}

bool Application::initGLFW() {
    if (!glfwInit()) {
        cerr << "GLFW initialization error" << endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    window = glfwCreateWindow(1400, 900, "Function Visualizer", NULL, NULL);
    if (!window) {
        cerr << "GLFW window creation error" << endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    g_ApplicationInstance = this;
    glfwSetWindowUserPointer(window, this);

    glfwSetMouseButtonCallback(window, Application::mouseButtonCallback);
    glfwSetScrollCallback(window, Application::scrollCallback);

    return true;
}

void Application::initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 120");
}

void Application::render() {
    if (isDragging) {
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        int width, height;
        glfwGetWindowSize(window, &width, &height);

        float dx = static_cast<float>((mouseX - lastMouseX) / width);
        float dy = static_cast<float>((mouseY - lastMouseY) / height);

        coordSystem.pan(-dx, dy);

        float xmin, xmax, ymin, ymax;
        coordSystem.getViewRange(xmin, xmax, ymin, ymax);
        plotter.setRange(xmin, xmax);

        rangeMin = xmin;
        rangeMax = xmax;
        lastMouseX = mouseX;
        lastMouseY = mouseY;
    }

    glClearColor(0.08f, 0.08f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    coordSystem.draw(window);
    plotter.draw();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

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

    ImGui::Separator();
    auto& functions = plotter.getFunctions();
    ImGui::Text("Functions (%d):", static_cast<int>(functions.size()));

    if (ImGui::BeginChild("FunctionList", ImVec2(400, 250), true)) {
        for (size_t i = 0; i < functions.size(); i++) {
            ImGui::PushID(static_cast<int>(i));

            // Sprawdzanie błędów dla aktualnej funkcji
            MathExpressionParser checker;
            checker.setExpression(functions[i].expression);
            string errorMsg = checker.getErrorMessage();

            // Wyświetlanie ikony błędu, jeśli występuje
            if (!errorMsg.empty()) {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "[!] ");
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", errorMsg.c_str());
                }
                ImGui::SameLine();
            }

            ImGui::ColorEdit3("##color", (float*)&functions[i].color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
            ImGui::SameLine();
            ImGui::Checkbox("##enabled", &functions[i].enabled);
            ImGui::SameLine();

            if (functions[i].editing) {
                char editBuffer[256];
                strcpy(editBuffer, functions[i].editBuffer.c_str());
                ImGui::PushItemWidth(150);
                if (ImGui::InputText("##edit", editBuffer, IM_ARRAYSIZE(editBuffer))) {
                    functions[i].editBuffer = editBuffer;
                }
                ImGui::PopItemWidth();
                
                

                ImGui::SameLine();
                if (ImGui::Button("V")) {
                    functions[i].applyEdit();
                    plotter.editFunction(static_cast<int>(i), functions[i].expression);
                }
                ImGui::SameLine();
                if (ImGui::Button("X")) {
                    functions[i].cancelEdit();
                }
                
                // Walidacja błędu na żywo podczas edycji
                MathExpressionParser live;
                live.setExpression(functions[i].editBuffer);
                if (!live.getErrorMessage().empty()) {
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Error: %s", live.getErrorMessage().substr(0, 30).c_str());
                }
            } else {
                ImGui::Text("%s", functions[i].expression.c_str());
                ImGui::SameLine();
                if (ImGui::Button("Edit")) {
                    functions[i].startEditing();
                }
                ImGui::SameLine();
                if (ImGui::Button("X")) {
                    plotter.removeFunction(static_cast<int>(i));
                    ImGui::PopID();
                    break;
                }
            }
            ImGui::PopID();
        }
    }
    ImGui::EndChild();

    if (ImGui::Button("Clear All")) { plotter.clear(); }

    ImGui::Separator();
    ImGui::Text("Zoom Controls:");
    if (ImGui::Button("Zoom In (+)", ImVec2(120, 25))) {
        float xmin, xmax, ymin, ymax;
        coordSystem.getViewRange(xmin, xmax, ymin, ymax);
        coordSystem.zoom(0.8f, (xmin + xmax) / 2.0f, (ymin + ymax) / 2.0f);
        coordSystem.getViewRange(xmin, xmax, ymin, ymax);
        plotter.setRange(xmin, xmax);
        rangeMin = xmin; rangeMax = xmax;
    }
    ImGui::SameLine();
    if (ImGui::Button("Zoom Out (-)", ImVec2(120, 25))) {
        float xmin, xmax, ymin, ymax;
        coordSystem.getViewRange(xmin, xmax, ymin, ymax);
        coordSystem.zoom(1.2f, (xmin + xmax) / 2.0f, (ymin + ymax) / 2.0f);
        coordSystem.getViewRange(xmin, xmax, ymin, ymax);
        plotter.setRange(xmin, xmax);
        rangeMin = xmin; rangeMax = xmax;
    }

    ImGui::Separator();
    ImGui::InputFloat("X min", &rangeMin);
    ImGui::InputFloat("X max", &rangeMax);
    if (ImGui::Button("Apply Range")) {
        if (rangeMin >= rangeMax) swap(rangeMin, rangeMax);
        plotter.setRange(rangeMin, rangeMax);
        coordSystem.setViewRange(rangeMin, rangeMax, -rangeMax, rangeMax);
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset View")) {
        rangeMin = -10.0f; rangeMax = 10.0f;
        plotter.setRange(rangeMin, rangeMax);
        coordSystem.resetView();
    }

    ImGui::End();

    if (showHelp) {
        ImGui::Begin("Help - Supported Functions", &showHelp, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::BulletText("Basic: y=x^2, y=2*x+3");
        ImGui::BulletText("Trig: sin(x), cos(x), tan(x)");
        ImGui::BulletText("Math: ln(x), log(x), e^x, abs(x)");
        ImGui::BulletText("Special: x=5 (vertical), x^2+y^2=9 (circle)");
        if (ImGui::Button("Close Help")) showHelp = false;
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
}

void Application::cleanup() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
    g_ApplicationInstance = nullptr;
}

void Application::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) app->onMouseButton(button, action, mods);
}

void Application::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) app->onScroll(xoffset, yoffset);
}

void Application::onMouseButton(int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            isDragging = true;
            glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
        } else if (action == GLFW_RELEASE) {
            isDragging = false;
        }
    }
}

void Application::onScroll(double xoffset, double yoffset) {
    (void)xoffset; (void)yoffset;
}

int Application::run() {
    if (!initGLFW()) return -1;
    initImGui();

    plotter.setRange(rangeMin, rangeMax);
    coordSystem.setViewRange(rangeMin, rangeMax, -rangeMax, rangeMax);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        render();
    }

    cleanup();
    return 0;
}
