// ShaderLoader.hpp / .cpp
#include <vulkan/vulkan.h>
#include <cstdio>
#include "LogError/LogError.hpp"

namespace ShaderLoader {
    // Временный буфер на стеке или в секции данных, если файлы шейдеров небольшие (обычно SPIR-V весят 2-10 КБ)
    // 64 КБ хватит для большинства базовых шейдеров движка
    constexpr size_t MAX_SHADER_SIZE = 65536; 

    inline VkShaderModule createShaderModule(VkDevice device, const char* filename) {
        FILE* file = fopen(filename, "rb");
        if (!file) {
            LOG_ERROR("[ERROR] Failed to open shader file!");
            return VK_NULL_HANDLE;
        }

        // Определяем размер файла
        fseek(file, 0, SEEK_END);
        size_t fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);
        
        ASSERT(fileSize <= MAX_SHADER_SIZE, "[ERROR] Shader file is too big for static buffer!");
        
        // Vulkan требует, чтобы код SPIR-V был выровнен по 4 байта (uint32_t)
        uint32_t buffer[MAX_SHADER_SIZE / 4]; 
        size_t readBytes = fread(buffer, 1, fileSize, file);
        fclose(file);

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = fileSize;
        createInfo.pCode = buffer;

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            LOG_ERROR("[ERROR] Failed to create shader module!");
            return VK_NULL_HANDLE;
        }

        return shaderModule;
    }
}
