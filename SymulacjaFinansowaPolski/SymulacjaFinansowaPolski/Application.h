#ifndef APPLICATION_H
#define APPLICATION_H

#include <GLFW/glfw3.h>
#include "imgui.h"
#include "MultiFunctionPlotter.h"
#include "CoordinateSystem.h"

class Application {
private:
    GLFWwindow* window;

    MultiFunctionPlotter plotter;
    CoordinateSystem coordSystem;

    char equationInput[256];
    float rangeMin, rangeMax;
    bool showHelp;
    bool isDragging;
    double lastMouseX, lastMouseY;

    bool initGLFW();
    void initImGui();
    void render();
    void cleanup();

    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

    void onMouseButton(int button, int action, int mods);
    void onScroll(double xoffset, double yoffset);

public:
    Application();
    ~Application();

    int run();
};

#endif
