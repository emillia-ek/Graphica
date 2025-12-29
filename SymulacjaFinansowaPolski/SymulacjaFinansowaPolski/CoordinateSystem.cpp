#include "CoordinateSystem.h"
#include <cmath>
#include <GLFW/glfw3.h>

using namespace std;

// Konstruktor
CoordinateSystem::CoordinateSystem() :
    viewXMin(-10.0f), viewXMax(10.0f), viewYMin(-10.0f), viewYMax(10.0f),
    baseXMin(-10.0f), baseXMax(10.0f), baseYMin(-10.0f), baseYMax(10.0f) {}

// Logika określania odstępu siatki (grid spacing)
float CoordinateSystem::getSpacing(float range) {
    if (range > 50.0f) return 5.0f;
    else if (range > 20.0f) return 2.0f;
    else if (range < 2.0f) return 0.2f;
    else if (range < 5.0f) return 0.5f;
    return 1.0f;
}

// OSTATECZNIE POPRAWIONA FUNKCJA: Rysowanie tylko strzałki na dodatnim X i Y, z ujednoliconymi proporcjami
void CoordinateSystem::drawArrows(float viewLeft, float viewRight, float viewBottom, float viewTop) {
    glColor3f(0.8f, 0.8f, 0.8f);

    float viewRangeX = viewRight - viewLeft;
    float viewRangeY = viewTop - viewBottom;
    
    float arrowLength = viewRangeX * 0.015f;
    float arrowWidth = viewRangeY * 0.015f / 2.0f;

    float ARROW_SCALE = 0.015f;
    float baseSizeX = (viewRight - viewLeft) * ARROW_SCALE;
    float baseSizeY = (viewTop - viewBottom) * ARROW_SCALE;
    
    float DLUGOSC_GROTA = baseSizeX;
    float POLOWA_SZEROKOSCI_GROTA = baseSizeY / 2.0f;


    glBegin(GL_TRIANGLES);

    // --- STRZAŁKA 1: Dodatni koniec osi X (Prawo) ---
    glVertex2f(viewRight, 0.0f);
    // Cofnięcie o DLUGOSC_GROTA, rozszerzenie o POLOWA_SZEROKOSCI_GROTA
    glVertex2f(viewRight - DLUGOSC_GROTA, POLOWA_SZEROKOSCI_GROTA);
    glVertex2f(viewRight - DLUGOSC_GROTA, -POLOWA_SZEROKOSCI_GROTA);

    // --- STRZAŁKA 2: Dodatni koniec osi Y (Góra) ---
    glVertex2f(0.0f, viewTop);
    // Cofnięcie o DLUGOSC_GROTA, rozszerzenie o POLOWA_SZEROKOSCI_GROTA
    glVertex2f(POLOWA_SZEROKOSCI_GROTA, viewTop - DLUGOSC_GROTA);
    glVertex2f(-POLOWA_SZEROKOSCI_GROTA, viewTop - DLUGOSC_GROTA);

    glEnd();
}

// Funkcja pomocnicza do rysowania znaczników na osiach
void CoordinateSystem::drawAxisLabels(float xSpacing, float ySpacing) {
    glColor3f(0.7f, 0.7f, 0.7f);

    int startX = static_cast<int>(ceil(viewXMin / xSpacing));
    int endX = static_cast<int>(floor(viewXMax / xSpacing));

    // Znaczniki na osi X
    for (int x = startX; x <= endX; x++) {
        if (x != 0) {
            glBegin(GL_LINES);
            glVertex2f(x * xSpacing, -0.1f);
            glVertex2f(x * xSpacing, 0.1f);
            glEnd();
        }
    }

    int startY = static_cast<int>(ceil(viewYMin / ySpacing));
    int endY = static_cast<int>(floor(viewYMax / ySpacing));

    // Znaczniki na osi Y
    for (int y = startY; y <= endY; y++) {
        if (y != 0) {
            glBegin(GL_LINES);
            glVertex2f(-0.1f, y * ySpacing);
            glVertex2f(0.1f, y * ySpacing);
            glEnd();
        }
    }
}

// Główna funkcja rysująca
void CoordinateSystem::draw(GLFWwindow* window) {
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    float aspect = (float)windowWidth / windowHeight;

    float currentWidth = viewXMax - viewXMin;
    float currentHeight = viewYMax - viewYMin;

    float centerX = (viewXMin + viewXMax) / 2.0f;
    float centerY = (viewYMin + viewYMax) / 2.0f;

    float targetWidth, targetHeight;

    // Korekta proporcji (aspect ratio)
    if (aspect > 1.0f) {
        targetHeight = currentHeight;
        targetWidth = targetHeight * aspect;
    } else {
        targetWidth = currentWidth;
        targetHeight = targetWidth / aspect;
    }
    
    // Granice widoku po korekcie proporcji
    float viewLeft = centerX - targetWidth / 2.0f;
        float viewRight = centerX + targetWidth / 2.0f;
        float viewBottom = centerY - targetHeight / 2.0f;
        float viewTop = centerY + targetHeight / 2.0f;

    lastViewLeft = viewLeft;
        lastViewRight = viewRight;
        lastViewBottom = viewBottom;
        lastViewTop = viewTop;
    
    // Ustawienie rzutowania ortogonalnego
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(viewLeft, viewRight, viewBottom, viewTop, -1.0f, 1.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Rysowanie siatki (cienkie linie)
    glColor3f(0.12f, 0.12f, 0.12f);
    glBegin(GL_LINES);

    float xSpacing = getSpacing(targetWidth);
    float ySpacing = getSpacing(targetHeight);

    float xStart = ceil(viewLeft / xSpacing) * xSpacing;
    float xEnd = viewRight;
    for (float x = xStart; x <= xEnd; x += xSpacing) {
        if (fabs(x) > 0.0001f) {
            glVertex2f(x, viewBottom);
            glVertex2f(x, viewTop);
        }
    }

    float yStart = ceil(viewBottom / ySpacing) * ySpacing;
    float yEnd = viewTop;
    for (float y = yStart; y <= yEnd; y += ySpacing) {
        if (fabs(y) > 0.0001f) {
            glVertex2f(viewLeft, y);
            glVertex2f(viewRight, y);
        }
    }

    glEnd();

    // Rysowanie pogrubionych osi
    glColor3f(0.4f, 0.4f, 0.4f);
    glLineWidth(1.5f);
    glBegin(GL_LINES);

    // X axis (do krawędzi widoku)
    glVertex2f(viewLeft, 0.0f);
    glVertex2f(viewRight, 0.0f);

    // Y axis (do krawędzi widoku)
    glVertex2f(0.0f, viewBottom);
    glVertex2f(0.0f, viewTop);

    glEnd();
    glLineWidth(1.0f);

    // Rysowanie strzałek (teraz z ujednoliconymi proporcjami)
    drawArrows(viewLeft, viewRight, viewBottom, viewTop);
    
    // Rysowanie znaczników
    drawAxisLabels(xSpacing, ySpacing);

    // Rysowanie głównych linii jednostkowych (ciemniejsze)
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_LINES);

    // Main horizontal lines
    for (float y = yStart; y <= yEnd; y += ySpacing) {
        if (fabs(y) > 0.0001f) {
            glVertex2f(viewLeft, y);
            glVertex2f(viewRight, y);
        }
    }

    // Main vertical lines
    for (float x = xStart; x <= xEnd; x += xSpacing) {
        if (fabs(x) > 0.0001f) {
            glVertex2f(x, viewBottom);
            glVertex2f(x, viewTop);
        }
    }

    glEnd();
}

// Reszta funkcji pozostaje bez zmian
void CoordinateSystem::setViewRange(float xmin, float xmax, float ymin, float ymax) {
    viewXMin = xmin;
    viewXMax = xmax;
    viewYMin = ymin;
    viewYMax = ymax;
}

void CoordinateSystem::zoom(float factor, float centerX, float centerY) {
    float xRange = viewXMax - viewXMin;
    float yRange = viewYMax - viewYMin;

    float newXRange = xRange * factor;
    float newYRange = yRange * factor;

    viewXMin = centerX - newXRange / 2.0f;
    viewXMax = centerX + newXRange / 2.0f;
    viewYMin = centerY - newYRange / 2.0f;
    viewYMax = centerY + newYRange / 2.0f;
}

void CoordinateSystem::pan(float dx, float dy) {
    float xRange = viewXMax - viewXMin;
    float yRange = viewYMax - viewYMin;

    viewXMin += dx * xRange;
    viewXMax += dx * xRange;
    viewYMin += dy * yRange;
    viewYMax += dy * yRange;
}

void CoordinateSystem::resetView() {
    viewXMin = baseXMin;
    viewXMax = baseXMax;
    viewYMin = baseYMin;
    viewYMax = baseYMax;
}

void CoordinateSystem::getViewRange(float& xmin, float& xmax, float& ymin, float& ymax) const {
    xmin = lastViewLeft;   // Musisz dodać te pola do klasy w .h
        xmax = lastViewRight;
        ymin = lastViewBottom;
        ymax = lastViewTop;
}

void CoordinateSystem::screenToGraph(GLFWwindow* window, int screenX, int screenY, float& graphX, float& graphY) {
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    float centerX = (viewXMin + viewXMax) / 2.0f;
    float centerY = (viewYMin + viewYMax) / 2.0f;
    float currentWidth = viewXMax - viewXMin;
    float currentHeight = viewYMax - viewYMin;

    float aspect = (float)width / height;
    float targetWidth, targetHeight;

    if (aspect > 1.0f) {
        targetHeight = currentHeight;
        targetWidth = targetHeight * aspect;
    } else {
        targetWidth = currentWidth;
        targetHeight = targetWidth / aspect;
    }

    graphX = centerX - targetWidth/2 + (screenX / (float)width) * targetWidth;
    graphY = centerY + targetHeight/2 - (screenY / (float)height) * targetHeight;
}
