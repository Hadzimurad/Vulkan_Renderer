#pragma once
#include <vulkan/vulkan.h>

enum class RenderPassType {
    Geometry3D, // Для рисования игры (делает CLEAR)
    ImGuiUI     // Для рисования интерфейса поверх игры (делает LOAD)
};

class VkRenderPassModule {
private:
    VkDevice       m_device     = VK_NULL_HANDLE;
    VkRenderPass   m_renderPass = VK_NULL_HANDLE;
    RenderPassType m_type       = RenderPassType::Geometry3D;

public:
    VkRenderPassModule() = default;
    ~VkRenderPassModule();

    // Запрет копирования
    VkRenderPassModule(const VkRenderPassModule&) = delete;
    VkRenderPassModule& operator=(const VkRenderPassModule&) = delete;

    // Инициализация принимает формат из Свопчейна и тип пасса
    bool Init(VkDevice device, VkFormat swapchainFormat, RenderPassType type);

    VkRenderPass get_render_pass() const { return m_renderPass; }
};
