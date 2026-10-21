#pragma once
#include <cstdint>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Window {
private:
    GLFWwindow* m_window;
    const uint32_t m_height = 0;
    const uint32_t m_width = 0;

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = nullptr;

public:
    Window(const uint32_t width = 0, const uint32_t height = 0);
    ~Window();
    // Запрет конструктора копирования
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    

    // Методы геттеры
    GLFWwindow* get_window();
    uint32_t get_ExtentionCount();
    const char** get_glfwExtention();
    inline uint32_t get_height() const { return m_height; }
    inline uint32_t get_width() const { return m_width; }

    // Методы чекеры
    inline bool should_close() const { return glfwWindowShouldClose(m_window); }
    inline void pollEvents() { glfwPollEvents(); }

};
