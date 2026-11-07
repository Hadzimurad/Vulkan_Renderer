#include "VkSurfaceModule.hpp"
#include "LogError/LogError.hpp"

bool VkSurfaceModule::Init(VkInstance instance, GLFWwindow* windowHandle) {
    m_instance = instance;

    // Тут вызывается твоя функция (например, через GLFW или Win32 напрямую)
    // Важно: в Orthodox C++ уходим от std::runtime_error и throw!
    if (glfwCreateWindowSurface(instance, windowHandle, nullptr, &m_surface) != VK_SUCCESS) {
        LOG_ERROR("Failed to create window surface!");
        return false; 
    }
    return true;
}

void VkSurfaceModule::cleanup() {
    if (m_surface != VK_NULL_HANDLE) {
        // Поверхность уничтожается через инстанс, поэтому мы запоминали его в m_instance
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    }   
}
