#include "VkDeviceModule.hpp"
#include "LogError/LogError.hpp"
#include <cstdint>
#include <cstdio>

// В анонимном пространстве имен объявляем расширения устройства, которые нам нужны (например, для Swapchain)
namespace {
    const char* deviceExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    constexpr uint32_t DEVICE_EXTENSIONS_COUNT = sizeof(deviceExtensions) / sizeof(deviceExtensions[0]);
}


QueueFamilyIndices VkDeviceModule::findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    ASSERT(queueFamilyCount <= 32, "[ERROR] Too many queue families on GPU!");
    VkQueueFamilyProperties queueFamilies[32];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);

    
    for (int i = 0; i < queueFamilyCount; ++i) {
        // Проверяем поддержку графики
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i; 
        }

        // Проверяем поддержку вывода на экран (презентации)
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }
        i++;
    }
    
    return indices;
}

bool VkDeviceModule::isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    // КРИТИЧЕСКИ ВАЖНО: Проверяем, что видеокарта поддерживает версию 1.2(на моей GTX 660)
    if (deviceProperties.apiVersion < VK_API_VERSION_1_2) { 
        return false;
    }

    QueueFamilyIndices indices = findQueueFamilies(device, surface);


    // Для GTX 660 нам важно, чтобы это была дискретная видеокарта (DISCRETE_GPU)
    bool isGpuTypeValid = (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU || 
                           deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU);
    
    return indices.isComplete() && isGpuTypeValid;
}

bool VkDeviceModule::Init(VkInstance instance, VkSurfaceKHR surface) {
    
    // Выбор Физического Устройства (Видеокарты)
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        LOG_ERROR("Failed to find GPUs with Vulkan support!");
        return false;
    }
    ASSERT(deviceCount <= 8, "[ERROR] To many GPUs on system!");
    VkPhysicalDevice devices[8];
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices);

    // Шаг 1: Сначала ищем строго дискретную карту (GTX 660 и т.д.)
    for (uint32_t i = 0; i < deviceCount; ++i) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(devices[i], &props);
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && isDeviceSuitable(devices[i], surface)) {
            m_physicalDevice = devices[i];
            break;
        }
    }

    // Шаг 2: Если дискретную не нашли, берем любую подходящую (например, встроенную)
    if (m_physicalDevice == VK_NULL_HANDLE) {
        for (uint32_t i = 0; i < deviceCount; ++i) {
            if (isDeviceSuitable(devices[i], surface)) {
                m_physicalDevice = devices[i];
                break;
            }
        }
    }

    if (m_physicalDevice == VK_NULL_HANDLE) {
        LOG_ERROR("Failed to find a suitable GPU (GTX 660 or equivalent with Vulkan 1.2)!");
        return false;
    }

    // Выведем имя нашей видеокарты, чтобы убедиться, что выбралась GTX 660
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &deviceProperties);
    
    printf("Selected GPU: %s", deviceProperties.deviceName);

    // 3. Создание Логического Устройства (VkDevice)
    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice, surface);

    VkDeviceQueueCreateInfo queueCreateInfos[2]{};
    uint32_t queueCreateCount = 0;


    float queuePriority = 1.0f; // Приоритет очереди (от 0.0 до 1.0)
    
    queueCreateInfos[queueCreateCount].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfos[queueCreateCount].queueFamilyIndex = indices.graphicsFamily;
    queueCreateInfos[queueCreateCount].queueCount = 1;
    queueCreateInfos[queueCreateCount].pQueuePriorities = &queuePriority;
    queueCreateCount++;

    if (indices.graphicsFamily != indices.presentFamily) {
        queueCreateInfos[queueCreateCount].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[queueCreateCount].queueFamilyIndex = indices.presentFamily;
        queueCreateInfos[queueCreateCount].queueCount = 1;
        queueCreateInfos[queueCreateCount].pQueuePriorities = &queuePriority;
        queueCreateCount++;
    }

    // Описываем фичи геометрии, которые нам пригодятся (например, анизотропная фильтрация)
    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE; 
 
    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = queueCreateCount;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    // ВАЖНЕЙШЕЕ ИСПРАВЛЕНИЕ: Включаем расширение Swapchain на уровне железа!
    deviceCreateInfo.enabledExtensionCount = DEVICE_EXTENSIONS_COUNT;
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

    // В современных версиях Vulkan устройство не требует слоев, но для старых драйверов укажем 
    deviceCreateInfo.enabledLayerCount = 0;
    deviceCreateInfo.ppEnabledLayerNames = nullptr;
    

    VkDevice device;
    if (vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS) {
        LOG_ERROR("Failed to create logical device!");
        return false;
    }
    m_device = device;

    // Получаем хэндлы (указатели) на сами очереди, куда мы будем слать команды 
    vkGetDeviceQueue(m_device, indices.graphicsFamily, 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, indices.presentFamily, 0, &m_presentQueue);
    
    return true;
}

VkDeviceModule::~VkDeviceModule() {
    if (m_device != VK_NULL_HANDLE) {
        vkDestroyDevice(m_device, nullptr);
    }
}
