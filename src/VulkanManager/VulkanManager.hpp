#pragma once
#include "../Core/window.hpp"
#include "../ECS/Components.hpp"
#include "../ECS/TransformComponents.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include "../ECS/World.hpp"
#include "Core/ImGuiLayer/ImGuiLayer.hpp"
#include <stdexcept>
#include "VulkanTypes.hpp"
#include <vector>
#include <string>
#include <cstring>
#include <memory>
#include <set>
#include <fstream>
#include <glm/glm.hpp>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE



// Слои валидации — наш главный инструмент отладки в Vulkan
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

class VulkanContext {
public:
    int init_vulkan_core(Window& window_vulkan);
    void draw_frame(const World& world, GuiLayer imgui);
    void cleanup();
    // 2. Метод создания буфера и заливки данных вершин
    void createVertexBuffer(const std::vector<Vertex> vertices);
    // Создание индексного буфера
    void createIndexBuffer(const std::vector<uint16_t> indices);
    void fillInitGUIStruct(VulkanGUICreateInfo& initImGuiStruct);
private:
    
    //Структуры для базовой инициализации
    VkInstance       instance_m         = VK_NULL_HANDLE;
    VkSurfaceKHR     surface_m          = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_m   = VK_NULL_HANDLE;
    VkDevice         device_m           = VK_NULL_HANDLE;
    VkCommandPool    commandPool_m      = VK_NULL_HANDLE;    

    //Командный буфер
    VkCommandBuffer  currentCommandBuffer_m;
    //Индексные буферы  
    VkBuffer m_indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_indexBufferMemory = VK_NULL_HANDLE; 

    //Вершинные буферы 
    VkBuffer         vertexBuffer_m     = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory_m = VK_NULL_HANDLE;


    // Структуры для организации кода
    VulkanQueue          m_queue{};
    VulkanSwapChain      m_swapChain{};
    VulkanPipeline       m_pipeline{};
    VulkanSyncPrimitives m_sync{};



    // Нахождение очередей графики и презентации на GPU 
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
    // Подтверждение поддержки свопчейна
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
    // Формат пикселей, режим вывода и разрешение окна 
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    // Поддержка портов очередей
    bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
    // Проверка поддержки слоев валидации
    bool checkValidationLayerSupport();
    // Чтения файла шейдера в формате SPIR-V
    std::vector<char> readFile(const std::string& filename);
    // Создание шейдерного модуля
    VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code);
    // Запсить команд в командный буфер
    void recordCommandBuffer(const World& world, uint32_t imageIndex, GuiLayer imgui);
    // Помошник для поиска памяти типа памяти на GPU
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};
