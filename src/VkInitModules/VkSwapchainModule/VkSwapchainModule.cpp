#include "VkSwapchainModule.hpp"
#include <cstdint>


constexpr uint32_t max(uint32_t a, uint32_t b) {
    if (a > b) {
        return a;
    }
    return b;
}

constexpr uint32_t min(uint32_t a, uint32_t b) {
    if (a < b) {
        return a;
    }
    return b;
}

// Выбираем формат пикселей. Идеально: BGRA 8-бит со стандартным цветовым пространством SRGB
VkSurfaceFormatKHR VkSwapchainModule::chooseSwapSurfaceFormat(VkSurfaceFormatKHR* availableFormats, size_t size) {
    
    for (size_t i = 0; i < size; ++i) {
        if (availableFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && 
            availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormats[i];
        }
    }
    return availableFormats[0]; // Если идеального нет, берем первый попавшийся
}

// Выбираем режим вывода. MAILBOX — это тройная буферизация (минимальный инпут-лаг, без разрывов)
VkPresentModeKHR VkSwapchainModule::chooseSwapPresentMode(VkPresentModeKHR* availablePresentModes, size_t size) {
    for (int i = 0; i < size; ++i) {
        if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentModes[i]; // Возвращаем тройную буферизацию, если она есть
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR; // FIFO есть ВСЕГДА — это стандартный V-Sync
}

// Выбираем разрешение кадров. Обычно это просто размер нашего окна GLFW (800x600)
VkExtent2D VkSwapchainModule::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, const uint32_t width, const uint32_t height) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        VkExtent2D actualExtent{width, height}; 
        actualExtent.width = max(capabilities.minImageExtent.width, min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = max(capabilities.minImageExtent.height, min(capabilities.maxImageExtent.height, actualExtent.height));
        return actualExtent;
    }
}

bool VkSwapchainModule::init(VkPhysicalDevice physicalDevice, VkDevice device, 
                             VkSurfaceKHR surface, const uint32_t height, const uint32_t width) {


    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, surface); 
 
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats, swapChainSupport.size_formats());
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes, swapChainSupport.size_presentModes());
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, height, width);
 
    m_swapchainInfo.format = surfaceFormat;
    m_swapchainInfo.presentMode = presentMode;
    m_swapchainInfo.extent = extent;

    LOG("[LOG 2] Chosen Format: %d, Mode: %d", surfaceFormat.format, presentMode);

    // Зануляем количество картинок до минимума для теста стабильности
    m_swapchainInfo.capabilitesMinImageCount = swapChainSupport.capabilities.minImageCount;

    m_swapchainInfo.capabilitesMaxImageCount = swapChainSupport.capabilities.minImageCount + 1;
    LOG("[LOG 3] Requested Image Count: %d", m_swapchainInfo.capabilitesMaxImageCount);

    VkSwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.pNext = nullptr;
    swapchainCreateInfo.flags = 0;
    swapchainCreateInfo.surface = surface;
    swapchainCreateInfo.minImageCount = m_swapchainInfo.capabilitesMinImageCount; // Минимально возможная двойная буферизация
    swapchainCreateInfo.imageFormat = surfaceFormat.format; // Должно быть 50 (B8G8R8A8_SRGB)
    swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent = extent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.queueFamilyIndexCount = 0;
    swapchainCreateInfo.pQueueFamilyIndices = nullptr;

    swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR; // Мы проверили, тут честная 1
    
    // ИСПРАВЛЕНИЕ: Пробуем INHERIT вместо OPAQUE на случай конфликта с темами Windows
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; 
    
    swapchainCreateInfo.presentMode = presentMode;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    LOG("[LOG 4] Calling vkCreateSwapchainKHR with Extent (800x600)...");
    
    if (vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &m_swapchain) != VK_SUCCESS) {
        LOG_ERROR("[ERROR] vkCreateSwapchainKHR failed");
        return false;
    }


    vkGetSwapchainImagesKHR(device, m_swapchain, &m_swapchainInfo.imageCount, nullptr);

    ASSERT(m_swapchainInfo.imageCount <= 8, "[ERROR] Swapchain allocated more than 8 images!");

    vkGetSwapchainImagesKHR(device, m_swapchain, &m_swapchainInfo.imageCount, m_swapchainInfo.images);

    LOG("[LOG 5] Swapchain created. Fetching images...");

    for (uint32_t i = 0; i < m_swapchainInfo.imageCount; ++i) {
        VkImageViewCreateInfo viewCreateInfo{};
        viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.image = m_swapchainInfo.images[i]; // Привязываем к i-й картинке
        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCreateInfo.format = m_swapchainInfo.format.format;

        // Настройка каналов (стандартное отображение)
        viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        // Описываем, что это цветовой буфер без мип-маппинга и слоев
        viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewCreateInfo.subresourceRange.baseMipLevel = 0;
        viewCreateInfo.subresourceRange.levelCount = 1;
        viewCreateInfo.subresourceRange.baseArrayLayer = 0;
        viewCreateInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &viewCreateInfo, nullptr, &m_swapchainInfo.imageViews[i]) != VK_SUCCESS) {
            LOG_ERROR("[ERROR] Failed to create image view for swapchain!");
            return false;
        }
    }
    m_device = device;
    return true;
}

SwapChainSupportDetails VkSwapchainModule::querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
    SwapChainSupportDetails details;
    // 1. Получаем базовые возможности
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);


    // 2. Получаем поддерживаемые форматы
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    ASSERT(formatCount != 0 && formatCount <= 64, "Surface Formats is failed");
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats);


    // 3. Получаем режимы вывода
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    ASSERT(presentModeCount != 0 && presentModeCount <= 64, "Surface Formats is failed"); 
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes);

    return details;
}

bool VkSwapchainModule::createFramebuffers(VkRenderPass renderPass) {
    // Бежим по всем картинкам, которые выделил нам свопчейн
    for (uint32_t i = 0; i < m_swapchainInfo.imageCount; ++i) {
        
        // Описываем, какие вьюшки привязаны к этому фреймбуферу
        VkImageView attachments[] = {
            m_swapchainInfo.imageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass; // Тот самый рендер пасс (игра или ImGui)
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = m_swapchainInfo.extent.width;   // Разрешение (например, 800)
        framebufferInfo.height = m_swapchainInfo.extent.height; // Разрешение (например, 600)
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapchainInfo.framebuffers[i]) != VK_SUCCESS) {
            LOG_ERROR("[ERROR] Failed to create framebuffer!");
            return false;
        }
    }
    return true;
}


VkSwapchainModule::~VkSwapchainModule() {
    if (m_device != VK_NULL_HANDLE) {
        for(uint32_t i = 0; i < m_swapchainInfo.imageCount; ++i) {
            if (m_swapchainInfo.framebuffers[i] != VK_NULL_HANDLE) {
                vkDestroyFramebuffer(m_device, m_swapchainInfo.framebuffers[i], nullptr);
            }
        }

        for(uint32_t i = 0; i < m_swapchainInfo.imageCount; ++i) {
            if (m_swapchainInfo.imageViews[i] != VK_NULL_HANDLE) {
                vkDestroyImageView(m_device, m_swapchainInfo.imageViews[i], nullptr);
            } 
        }

        if (m_swapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
        }
    }
}


