#pragma once
#include <cstdint>
#include <vulkan/vulkan.h>
#include "LogError/LogError.hpp"

// Сгруппированные ресурсы
struct VulkanSwapChainInfo {
    VkImage images[8];
    VkImageView imageViews[8];
    VkFramebuffer framebuffers[8];

    VkExtent2D extent = {}; 
    VkSurfaceFormatKHR format = {};
    VkPresentModeKHR presentMode = {};

    uint32_t capabilitesMinImageCount = 0;
    uint32_t capabilitesMaxImageCount = 0;
    
    uint32_t imageCount = 0;
};

// Детали SwapChain
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR formats[64];
    VkPresentModeKHR presentModes[64];

    size_t size_formats() { return sizeof(formats) / sizeof(formats[0]); }
    size_t size_presentModes() { return sizeof(presentModes) / sizeof(presentModes[0]); }
};

class VkSwapchainModule {
private:
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VulkanSwapChainInfo m_swapchainInfo = {};
    
    // Подтверждение поддержки свопчейна
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
    // Выбирает формат вывода пикслей
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(VkSurfaceFormatKHR* availableFormats, size_t size);
    VkPresentModeKHR chooseSwapPresentMode(VkPresentModeKHR* availablePresentModes, size_t size);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, const uint32_t height = 0, const uint32_t width = 0);

public:
    VkSwapchainModule() = default;
    ~VkSwapchainModule() = default;
    void cleanup();
    VkSwapchainModule(const VkSwapchainModule&) = delete;
    VkSwapchainModule& operator=(const VkSwapchainModule&) = delete;

    bool init(VkPhysicalDevice physycalDevice, VkDevice device, 
              VkSurfaceKHR surface, const uint32_t height = 0, const uint32_t width = 0);
    bool createFramebuffers(VkRenderPass renderPass); 
    VkSurfaceFormatKHR get_surfaceFormat() { return m_swapchainInfo.format; }
    VkPresentModeKHR get_presentMode() { return m_swapchainInfo.presentMode; }
    VkExtent2D get_extent2D() { return m_swapchainInfo.extent; }
    VkSwapchainKHR get_swapchain() { return m_swapchain; }
    inline VkFramebuffer get_framebuffer(uint32_t index) const { 
        ASSERT(index < m_swapchainInfo.imageCount, "[ERROR] Framebuffer index out of bounds!");
        return m_swapchainInfo.framebuffers[index]; 
    }
     
};