#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <array>

struct VulkanGUICreateInfo {
    VkInstance instance;
    VkPhysicalDevice physicalDevice; 
    VkDevice device;
    uint32_t queueFamily;
    VkQueue queue;
    VkRenderPass renderPass; 
    uint32_t minImageCount; 
    uint32_t imageCount;
};

// Очереди
struct QueueFamilyIndices {
    uint32_t graphicsFamily;
    uint32_t presentFamily;
    bool hasGraphics = false;
    bool hasPresent = false;

    bool isComplete() {
        return hasGraphics && hasPresent;
    }
};

//Структура для хранения Очередей и их индексов
struct VulkanQueue {
    QueueFamilyIndices indices{};
    VkQueue          graphicsQueue = VK_NULL_HANDLE;
    VkQueue          presentQueue = VK_NULL_HANDLE;
};

// Детали SwapChain
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

// Сгруппированные ресурсы
struct VulkanSwapChain {
    VkSwapchainKHR handle = VK_NULL_HANDLE;
    VkFormat imageFormat{};
    VkExtent2D extent{};
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    std::vector<VkFramebuffer> framebuffers;
    uint32_t minImageCount = 0;
    uint32_t maxImageCount = 0;
};

struct VulkanPipeline {
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkPipeline graphicsHandle = VK_NULL_HANDLE;
};

struct VulkanSyncPrimitives {
    VkSemaphore imageAvailable = VK_NULL_HANDLE;
    VkSemaphore renderFinished = VK_NULL_HANDLE;
};

// Данные Вершины
struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);
        return attributeDescriptions;
    }
};
