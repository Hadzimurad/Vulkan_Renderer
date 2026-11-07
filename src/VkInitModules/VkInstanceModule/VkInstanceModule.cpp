#include "VkInstanceModule.hpp"
#include <vulkan/vulkan.h>
#include <cstdint>
#include <cstring>
#include "LogError/LogError.hpp"

namespace VkValidations {
    // Слои валидации — наш главный инструмент отладки в Vulkan
    const char* validationLayers[] = {
        "VK_LAYER_KHRONOS_validation"
    };
    // Считаем количество элементов на этапе компиляции
    constexpr uint32_t VALIDATION_LAYERS_COUNT = sizeof(validationLayers) / sizeof(validationLayers[0]);
}

// Проверка поддержки слоев валидации
bool VkInstanceModule::checkValidationLayerSupport() {
    uint32_t layerCount;
    // Получаем все доступные слои
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    ASSERT(layerCount <= 128, "[ERROR] Too many validation layers on system!");
    // Массив в котором все доступные на GPU слои
    VkLayerProperties availableLayers[128];
    // Тут он иницализируется названиями доступных слоев
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

    // Ищем все нужные слои в validationLayers если хотя бы одного нет return false
    for (const char* layerName : VkValidations::validationLayers) {
        bool layerFound = false;
        for (const auto& layerProperties : availableLayers) {
            // сравниваем строки если равны return 0;
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        if (!layerFound) return false;
    }
    return true;
}

bool VkInstanceModule::Init(bool enableValidationLayers, uint32_t glfwExtentionCount, const char** glfwExtention) {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "DOD Engine";
    appInfo.apiVersion = VK_API_VERSION_1_2; 

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    
    createInfo.enabledExtensionCount = glfwExtentionCount;
    createInfo.ppEnabledExtensionNames = glfwExtention;

    if (enableValidationLayers) {
        if (!checkValidationLayerSupport()) {
            LOG_ERROR("Validation Layer is don`t suport");
            return false;
        }
        createInfo.enabledLayerCount = VkValidations::VALIDATION_LAYERS_COUNT;
        createInfo.ppEnabledLayerNames = VkValidations::validationLayers;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        LOG_ERROR("Failed to create instance");
        return false; 
    }
    return true;
}


VkInstance VkInstanceModule::get_instance() const { return instance; }


void VkInstanceModule::cleanup() {
    if (instance != VK_NULL_HANDLE) {
        vkDestroyInstance(instance, nullptr);
    }
}
