#pragma once
#include <vulkan/vulkan.h>
#include "LogError/LogError.hpp"

// Очереди
struct QueueFamilyIndices {
    uint32_t graphicsFamily = 0xFFFFFFFF;
    uint32_t presentFamily = 0xFFFFFFFF;

    bool isComplete() {
        return graphicsFamily != 0xFFFFFFFF && presentFamily != 0xFFFFFFFF;
    }
};



class VkDeviceModule {
private:
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;

    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    uint32_t m_graphicIndices = 0XFFFFFFF;
    uint32_t m_presentIndices = 0XFFFFFFF;
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
    bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);

public:

    VkDeviceModule() = default;
    ~VkDeviceModule() = default; 
    void cleanup();

    // Запрет копирования
    VkDeviceModule(const VkDeviceModule&) = delete;
    VkDeviceModule& operator=(const VkDeviceModule&) = delete;

    // Передаем инстанс и поверхность для проверки совместимости
    bool Init(VkInstance instance, VkSurfaceKHR surface);

    // Геттеры
    VkDevice         get_device() const          { return m_device; }
    VkPhysicalDevice get_physical_device() const { return m_physicalDevice; }
    VkQueue          get_graphics_queue() const  { return m_graphicsQueue; }
    VkQueue          get_present_queue() const   { return m_presentQueue; }
    uint32_t get_graphic_family_indices() const {
        ASSERT(m_graphicIndices != 0XFFFFFFF, "Graphic Indices is NULL");
        return m_graphicIndices;
    }
    uint32_t get_present_family_indices() const {
        ASSERT(m_presentIndices != 0XFFFFFFF, "Present Indices is NULL");
        return m_presentIndices;
    }

    
};