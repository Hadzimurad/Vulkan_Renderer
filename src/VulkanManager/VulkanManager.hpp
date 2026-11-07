#pragma once
#include "Core/window.hpp"
#include "ECS/Components.hpp"
//#include "ECS/TransformComponents.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include "ECS/World.hpp"
#include "Core/ImGuiLayer/ImGuiLayer.hpp"
#include "VkInitModules/VkInstanceModule/VkInstanceModule.hpp"
#include "VkInitModules/VkDeviceModule/VkDeviceModule.hpp"
#include "VkInitModules/VkRenderPassModule/VkRenderPassModule.hpp"
#include "VkInitModules/VkSurfaceModule/VkSurfaceModule.hpp"
#include "VkInitModules/VkSwapchainModule/VkSwapchainModule.hpp"
#include "VkInitModules/VkPipelineModule/VkPipelineModule.hpp"
#include "VulkanTypes.hpp"
#include <cstring>
#include <glm/glm.hpp>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

struct FrameData {
    VkCommandPool   commandPool             = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer           = VK_NULL_HANDLE;
    VkSemaphore     imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore     renderFinishedSemaphore = VK_NULL_HANDLE;
    VkFence         inFlightFence           = VK_NULL_HANDLE;
};

class VulkanContext {
public:
    bool init_vulkan_core(Window& window_vulkan);
    void draw_frame(const World& world, GuiLayer& imgui, uint32_t index);
    void cleanup();
    // 2. Метод создания буфера и заливки данных вершин
    bool createVertexBuffer(const Vertex* vertices);
    // Создание индексного буфера
    //void createIndexBuffer(const std::vector<uint16_t> indices);
    void fillInitGUIStruct(VulkanGUICreateInfo& initImGuiStruct);
private:
    
    FrameData m_frames[MAX_FRAMES_IN_FLIGHT];
    uint32_t m_currentFrameIndex = 0;

    VkInstanceModule    m_instance;
    VkSurfaceModule     m_surface;
    VkDeviceModule      m_device;
    VkSwapchainModule   m_swapchain;
    VkRenderPassModule  m_geometryRenderPass;
    VkRenderPassModule  m_imGuiRenderPass;
    VkPipelineModule m_pipeline{};


    //Вершинные буферы 
    VkBuffer         vertexBuffer_m     = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory_m = VK_NULL_HANDLE;

    // Функция для инициализации 
    bool init_commands_and_sync();
    // Запсить команд в командный буфер
    //void recordCommandBuffer(const World& world, uint32_t imageIndex, GuiLayer& imgui);
    // Помошник для поиска памяти типа памяти на GPU
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};
