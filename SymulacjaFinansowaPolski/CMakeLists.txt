cmake_minimum_required(VERSION 3.10)
project(FunctionVisualizer CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

# If glfw3 CONFIG is not found, fallback to find_package(glfw REQUIRED) or pkg_check_modules
# It's better to just try with glfw3, usually installed via Homebrew on Mac

set(SRC_DIR "SymulacjaFinansowaPolski")
set(IMGUI_DIR "${SRC_DIR}/libs/imgui")

set(SOURCES
    ${SRC_DIR}/Application.cpp
    ${SRC_DIR}/CoordinateSystem.cpp
    ${SRC_DIR}/FunctionData.cpp
    ${SRC_DIR}/FunctionPlotter3D.cpp
    ${SRC_DIR}/MathExpressionParser.cpp
    ${SRC_DIR}/MultiFunctionPlotter.cpp
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/glad.c
)

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES} ${IMGUI_SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${SRC_DIR}
    ${SRC_DIR}/include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# Mac OS specific frameworks
if (APPLE)
    find_library(COREVIDEO CoreVideo)
    find_library(IOKIT IOKit)
    find_library(COCOA Cocoa)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${COREVIDEO} ${IOKIT} ${COCOA})
endif()

# Libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    OpenGL::GL
)
