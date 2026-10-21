#include "VkRenderPassModule.hpp"
#include "LogError/LogError.hpp"

bool VkRenderPassModule::Init(VkDevice device, VkFormat swapchainFormat, RenderPassType type) {
    m_device = device;
    m_type = type;

    // 1. Описываем наш буфер цвета (куда будем рисовать)
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchainFormat; // Формат должен строго совпадать со Свопчейном!
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    
    // Главная настройка: Выбираем поведение в зависимости от назначения пасса
    if (m_type == RenderPassType::Geometry3D) {
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Стираем прошлый кадр игры
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Нам плевать, что было в памяти до этого
    } else {
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;   // Сохраняем графику игры под ImGui!
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Ждем, что там уже лежит готовая картинка
    }

    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // В обоих случаях сохраняем результат
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    
    // Финальный layout: 
    // Наша геометрия передает управление дальше, поэтому оставляет буфер в режиме COLOR_ATTACHMENT.
    // А вот ImGui — это ПОСЛЕДНИЙ пасс в кадре, после него картинка идет сразу на экран, 
    // поэтому ставим PRESENT_SRC_KHR
    if (m_type == RenderPassType::Geometry3D) {
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    } else {
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }

    // 2. Создаем ссылку на аттачмент для Subpass
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0; // Индекс в массиве (у нас он один)
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // 3. Описываем Subpass (хотя бы один обязан быть)
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    // 4. Настройка зависимостей подсистем (Subpass Dependencies)
    // Это нужно, чтобы пассы не начали выполняться одновременно и не устроили кашу в памяти.
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    // 5. Собираем структуру создания Render Pass
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
        LOG_ERROR("Failed to create Render Pass!");
        return false;
    }
    return true;
}

VkRenderPassModule::~VkRenderPassModule() {
    if (m_renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(m_device, m_renderPass, nullptr);
    }
}
