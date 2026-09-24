#include "window.hpp"
#include <GLFW/glfw3.h>
#include <stdexcept>

Window::Window() {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to init glfw");
    } 

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan DOD Engine", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Failed to init window");
    } 
    m_window = window;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    if (!glfwExtensions) {
        glfwDestroyWindow(m_window); // Чистим окно
        glfwTerminate(); // Чистим GLFW
        throw std::runtime_error("Failed to require GLFW instance extensions!");
    }
    return;

    std::cout << "Widow created" << std::endl;

}


bool Window::CreateWindowSurface(VkInstance instance, VkSurfaceKHR* surface) {
    if (glfwCreateWindowSurface(instance, m_window, nullptr, surface) != VK_SUCCESS) {
        std::cerr << "Failed to create window surface!" << std::endl;
        return 0; 
    }
    return 1;
}

bool Window::should_close() const { return glfwWindowShouldClose(m_window); }

GLFWwindow* Window::get_window() { return m_window; }

uint32_t Window::get_ExtentionCount() { return glfwExtensionCount; }

const char** Window::get_glfwExtention() { return glfwExtensions; }

void Window::pollEvents() { glfwPollEvents(); }

Window::~Window() {
    glfwDestroyWindow(m_window);
    glfwTerminate();
}