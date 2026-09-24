#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>

class Window {
private:
    GLFWwindow* m_window;
    bool init_succes = false;
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = nullptr;
public:
    Window();
    bool CreateWindowSurface(VkInstance instance, VkSurfaceKHR* surface);
    bool should_close() const;
    void pollEvents();
    // Запрет конструктора копирования
    Window(const Window&) = delete;
    // Запрет оператора копирующего присваивания (настоятельно рекомендуется запретить тоже)
    Window& operator=(const Window&) = delete;
    ~Window();

    // Методы геттеры и чеккеры
    GLFWwindow* get_window();
    uint32_t get_ExtentionCount();
    const char** get_glfwExtention();
};
