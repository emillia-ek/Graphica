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

    // Store application instance for callbacks
    g_ApplicationInstance = this;
    glfwSetWindowUserPointer(window, this);

    // Set callbacks
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

    ImGui::Text("Quick Add:");

    if (ImGui::Button("y=x")) { strcpy(equationInput, "y=x"); plotter.addFunction(equationInput); strcpy(equationInput, ""); }
    ImGui::SameLine();
    if (ImGui::Button("y=sin(x)")) { strcpy(equationInput, "y=sin(x)"); plotter.addFunction(equationInput); strcpy(equationInput, ""); }
    ImGui::SameLine();
    if (ImGui::Button("y=x^2")) { strcpy(equationInput, "y=x^2"); plotter.addFunction(equationInput); strcpy(equationInput, ""); }

    if (ImGui::Button("y=sin(2x)")) { strcpy(equationInput, "y=sin(2x)"); plotter.addFunction(equationInput); strcpy(equationInput, ""); }
    ImGui::SameLine();
    if (ImGui::Button("y=x/2")) { strcpy(equationInput, "y=x/2"); plotter.addFunction(equationInput); strcpy(equationInput, ""); }
    ImGui::SameLine();
    if (ImGui::Button("(x-2)^2+(y+1)^2=9")) { strcpy(equationInput, "(x-2)^2+(y+1)^2=9"); plotter.addFunction(equationInput); strcpy(equationInput, ""); }

    ImGui::Separator();

    auto& functions = plotter.getFunctions();
    ImGui::Text("Functions (%d):", static_cast<int>(functions.size()));

    if (ImGui::BeginChild("FunctionList", ImVec2(0, 200), true)) {
        for (size_t i = 0; i < functions.size(); i++) {
            ImGui::PushID(static_cast<int>(i));

            ImGui::ColorEdit3("##color", (float*)&functions[i].color,
                             ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
            ImGui::SameLine();

            ImGui::Checkbox("##enabled", &functions[i].enabled);
            ImGui::SameLine();

            if (functions[i].editing) {
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
                    plotter.editFunction(static_cast<int>(i), functions[i].expression);
                }
                ImGui::SameLine();
                if (ImGui::Button("X")) {
                    functions[i].cancelEdit();
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

    if (ImGui::Button("Clear All")) {
        plotter.clear();
    }

    ImGui::Separator();

    // Zoom buttons section
    ImGui::Text("Zoom Controls:");

    ImGui::Columns(2, "zoomcols", false);
    ImGui::SetColumnWidth(0, 150);

    // Zoom in button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
    if (ImGui::Button("Zoom In (+)", ImVec2(120, 30))) {
        float xmin, xmax, ymin, ymax;
        coordSystem.getViewRange(xmin, xmax, ymin, ymax);
        float centerX = (xmin + xmax) / 2.0f;
        float centerY = (ymin + ymax) / 2.0f;

        // Zoom in by 20%
        coordSystem.zoom(0.8f, centerX, centerY);
        coordSystem.getViewRange(xmin, xmax, ymin, ymax);
        plotter.setRange(xmin, xmax);
        rangeMin = xmin;
        rangeMax = xmax;
    }
    ImGui::PopStyleColor(2);

    ImGui::NextColumn();

    // Zoom out button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.3f, 0.3f, 1.0f));
    if (ImGui::Button("Zoom Out (-)", ImVec2(120, 30))) {
        float xmin, xmax, ymin, ymax;
        coordSystem.getViewRange(xmin, xmax, ymin, ymax);
        float centerX = (xmin + xmax) / 2.0f;
        float centerY = (ymin + ymax) / 2.0f;

        // Zoom out by 20%
        coordSystem.zoom(1.2f, centerX, centerY);
        coordSystem.getViewRange(xmin, xmax, ymin, ymax);
        plotter.setRange(xmin, xmax);
        rangeMin = xmin;
        rangeMax = xmax;
    }
    ImGui::PopStyleColor(2);

    ImGui::Columns(1);

    ImGui::Separator();
    ImGui::Text("Graph Range:");

    if (ImGui::InputFloat("X min", &rangeMin)) {}
    if (ImGui::InputFloat("X max", &rangeMax)) {}

    if (ImGui::Button("Apply Range")) {
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
    ImGui::BulletText("Left drag: Pan (move the graph)");
    ImGui::BulletText("+/- buttons: Zoom in/out (20% each click)");
    ImGui::BulletText("Click 'Edit' to modify functions");
    ImGui::BulletText("V to save, X to cancel editing");
    ImGui::BulletText("Grid maintains 1:1 aspect ratio");

    ImGui::End();

    if (showHelp) {
        ImGui::Begin("Help - Supported Functions", &showHelp, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text("SUPPORTED FUNCTIONS:");
        ImGui::Separator();
        ImGui::BulletText("Basic: y=x, y=x^2, y=2*x+3");
        ImGui::BulletText("Flipping: y=-f(x) for any f(x)");
        ImGui::BulletText("Fractions: y=x/2, y=(x+1)/3, y=1/2*x");
        ImGui::BulletText("Trig: sin(x), cos(x), tan(x), cot(x)");
        ImGui::BulletText("Exponential: y=e^x, y=2^x");
        ImGui::BulletText("Logarithmic: y=ln(x), y=log(x)");
        ImGui::BulletText("Circles: x^2+y^2=r^2, (x-a)^2+(y-b)^2=r^2");
        ImGui::BulletText("Lines: x=c, y=c");
        ImGui::BulletText("Polynomials: y=x^3-2x, y=x^4+3x^2-1");
        ImGui::BulletText("Implicit multiplication: y=sin(2x), y=2sin(x)");

        ImGui::Separator();
        ImGui::Text("EXAMPLES:");
        ImGui::BulletText("y=x^2 (parabola)");
        ImGui::BulletText("y=sin(x) (sine wave)");
        ImGui::BulletText("y=sin(2x) (compressed sine wave)");
        ImGui::BulletText("y=-x^2 (flipped parabola)");
        ImGui::BulletText("y=x/2 (line with slope 0.5)");
        ImGui::BulletText("(x-2)^2+(y+1)^2=9 (circle at (2,-1) radius 3)");

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
    if (app) {
        app->onMouseButton(button, action, mods);
    }
}

void Application::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->onScroll(xoffset, yoffset);
    }
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
    (void)mods;
}

void Application::onScroll(double xoffset, double yoffset) {
    // Optional: You can keep this empty or implement scroll zoom if needed
    (void)xoffset;
    (void)yoffset;
}

int Application::run() {
    if (!initGLFW()) return -1;

    initImGui();

    // Initial setup
    plotter.setRange(rangeMin, rangeMax);
    coordSystem.setViewRange(rangeMin, rangeMax, -rangeMax, rangeMax);

    // Add some example functions
    plotter.addFunction("y=x");
    plotter.addFunction("y=sin(x)");
    plotter.addFunction("y=x^2");
    plotter.addFunction("(x-2)^2+(y+1)^2=9");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        render();
    }

    cleanup();
    return 0;
}