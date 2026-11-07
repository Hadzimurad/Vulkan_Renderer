// VkPipelineModule.hpp
#pragma once
#include <vulkan/vulkan.h>

class VkPipelineModule {
private:
    VkDevice         m_device         = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline       m_graphicsPipeline = VK_NULL_HANDLE;

public:
    VkPipelineModule() = default;
    ~VkPipelineModule() = default;
    VkPipelineModule(const VkPipelineModule&) = delete;
    VkPipelineModule& operator=(const VkPipelineModule&) = delete;

    // Передаем то, от чего конвейер жестко зависит
    bool Init(VkDevice device, VkRenderPass renderPass, const char* vertPath, const char* fragPath);
    void cleanup();

    VkPipeline       get_pipeline() const { return m_graphicsPipeline; }
    VkPipelineLayout get_layout()   const { return m_pipelineLayout; }
};
