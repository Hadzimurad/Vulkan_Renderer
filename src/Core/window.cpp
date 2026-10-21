#include "window.hpp"
#include "LogError/LogError.hpp"
#include <GLFW/glfw3.h>
#include <cstdint>

Window::Window(const uint32_t width, const uint32_t height) : m_width(width), m_height(height) {
    if (width == 0 && height == 0 || !glfwInit()) {
        LOG("Failed to init glfw");
        return;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(width, height, "Vulkan DOD Engine", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        LOG("Failed to init window");
        return;
    } 
    m_window = window;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    if (!glfwExtensions) {
        glfwDestroyWindow(m_window); // Чистим окно
        glfwTerminate(); // Чистим GLFW
        LOG("Failed to require GLFW instance extensions!");
        return;
    }
    LOG("Widow created");
    return;
}

GLFWwindow* Window::get_window() { return m_window; }

uint32_t Window::get_ExtentionCount() { return glfwExtensionCount; }

const char** Window::get_glfwExtention() { return glfwExtensions; }


Window::~Window() {
    glfwDestroyWindow(m_window);
    glfwTerminate();
}