#include "ImGuiLayer.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <stdexcept>
#include <vector>

void GuiLayer::Init(GLFWwindow* window, VulkanGUICreateInfo imGuiInfo) {
    m_Device = imGuiInfo.device;

    // 1. Создаем Descriptor Pool специально для нужд ImGui
    // Добавляем SAMPLER и другие типы, которые могут потребоваться бэкенду ImGui
    VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },                // <-- Добавлено для исправления ошибки!
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000 * static_cast<uint32_t>(std::size(pool_sizes));
    pool_info.poolSizeCount = static_cast<uint32_t>(std::size(pool_sizes));
    pool_info.pPoolSizes = pool_sizes;

    if (vkCreateDescriptorPool(m_Device, &pool_info, nullptr, &m_DescriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("GuiLayer: Не удалось создать Descriptor Pool для ImGui!");
    }
    
      // 2. Инициализируем контекст самого ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    // Включаем поддержку Docking (вкладок), раз уж мы скачали эту ветку
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; 

    // Стилизация (Темная тема — классика)
    ImGui::StyleColorsDark();

    // 3. Инициализируем бэкенд для GLFW
    ImGui_ImplGlfw_InitForVulkan(window, true);

    // 4. Инициализируем бэкенд для Vulkan
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.PipelineInfoForViewports.RenderPass = imGuiInfo.renderPass;
    init_info.UseDynamicRendering = false;
    init_info.Instance = imGuiInfo.instance;
    init_info.PhysicalDevice = imGuiInfo.physicalDevice;
    init_info.Device = m_Device;
    init_info.QueueFamily = imGuiInfo.queueFamily;
    init_info.Queue = imGuiInfo.queue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = m_DescriptorPool;
    init_info.MinImageCount = imGuiInfo.minImageCount;
    init_info.ImageCount = imGuiInfo.imageCount;
    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = [](VkResult err) { 
        if (err != VK_SUCCESS) throw std::runtime_error("ImGui Vulkan Error!"); 
    };
    ImGui_ImplVulkan_Init(&init_info);
    

    ImGui_ImplVulkan_CreateFontsTexture();
    
}

void GuiLayer::BeginFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void GuiLayer::EndFrame() {
    ImGui::Render();
}

void GuiLayer::Render(VkCommandBuffer commandBuffer) {
    // Эта функция сама запишет все VkCmdDraw для интерфейса
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

void GuiLayer::Shutdown() {
    // Ждем, пока GPU дорендерит кадры, перед удалением
    vkDeviceWaitIdle(m_Device);

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (m_DescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
    }
}

GuiLayer::~GuiLayer() {
    this->Shutdown();
}
