#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "VulkanManager/VulkanTypes.hpp"

class GuiLayer {
public:
    GuiLayer() = default;
    ~GuiLayer();
    
    // Инициализация ресурсов для ImGui(Контекст, GLWF, и Vulkan Структур)
    void Init(GLFWwindow* window, VulkanGUICreateInfo imGuiInfo);
    
    //Начало нового кадра, вызывается в начале цикла отрисовки
    void BeginFrame();
    
    // Конец кадра и генерация вершин, вызывается после того, как описали Gui
    void EndFrame();
    
    // Запись команд отрисовки в буфер команд
    // Вызывается внутри вашего vkBeginRenderPass / vkEndRenderPass
    void Render(VkCommandBuffer commandBuffer);
    void Shutdown();

    
private:
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;
    VulkanGUICreateInfo guiCreateInfo{};
};
