#pragma once
#include <vulkan/vulkan.h>


class VkInstanceModule {
private:
    VkInstance instance = VK_NULL_HANDLE;
    // Проверка поддержки слоев валиадции
    bool checkValidationLayerSupport();

public:
    VkInstanceModule() = default;
    VkInstanceModule(const VkInstanceModule&) = delete;
    VkInstanceModule& operator=(const VkInstanceModule&) = delete;
    ~VkInstanceModule();
    
    bool Init(bool enableValidationLayers, uint32_t glfwExtentionCount, const char** glfwExtention); 
    VkInstance get_instance() const;
};
