#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

class VkSurfaceModule {
private:
    VkInstance   m_instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface  = VK_NULL_HANDLE;

public:
    VkSurfaceModule() = default;
    ~VkSurfaceModule() = default;
    void cleanup();

    // Запрещаем копирование
    VkSurfaceModule(const VkSurfaceModule&) = delete;
    VkSurfaceModule& operator=(const VkSurfaceModule&) = delete;

    // Вместо throw используем твой булев возврат и макрос ASSERT/LOG_ERROR
    bool Init(VkInstance instance, GLFWwindow* windowHandle); 
    
    VkSurfaceKHR get_surface() const { return m_surface; }
};
