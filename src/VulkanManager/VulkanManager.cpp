#include "VulkanManager.hpp"
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include "Core/ImGuiLayer/ImGuiLayer.hpp"
#include "LogError/LogError.hpp"
#include "VkInitModules/VkInstanceModule/VkInstanceModule.hpp"
#include "VkInitModules/VkSurfaceModule/VkSurfaceModule.hpp"
#include "VkInitModules/VkDeviceModule/VkDeviceModule.hpp"
#include "VkInitModules/VkSwapchainModule/VkSwapchainModule.hpp"
#include "VkInitModules/VkRenderPassModule/VkRenderPassModule.hpp"
#include "VkInitModules/VkPipelineModule/VkPipelineModule.hpp"

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

 void VulkanContext::fillInitGUIStruct(VulkanGUICreateInfo& GUIcreateInfo) {
     GUIcreateInfo.renderPass = m_imGuiRenderPass.get_render_pass();
     GUIcreateInfo.instance = m_instance.get_instance();
     GUIcreateInfo.device = m_device.get_device();
     GUIcreateInfo.physicalDevice = m_device.get_physical_device();
     GUIcreateInfo.imageCount = 3;
     GUIcreateInfo.minImageCount = 2;
     GUIcreateInfo.queue = m_device.get_graphics_queue(); 
     GUIcreateInfo.queueFamily = m_device.get_graphic_family_indices();
 }



/*
void VulkanContext::recordCommandBuffer(const World& world, uint32_t imageIndex, GuiLayer& imgui) {
    // Начало записи буфера
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(currentCommandBuffer_m, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    // Конфигурируем очистку экрана: выставляем темно-серый цвет фона (0.1, 0.1, 0.1)
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_pipeline.renderPass;
    renderPassInfo.framebuffer = m_swapChain.framebuffers[imageIndex]; // Рисуем в конкретный фреймбуфер текущего кадра
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_swapChain.extent;

    VkClearValue clearColor = {{{0.1f, 0.1f, 0.1f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
#include <stdexcept>
    renderPassInfo.pClearValues = &clearColor;

    // 1. Начинаем проход рендеринга (в этот момент видеокарта очистит экран)
    vkCmdBeginRenderPass(currentCommandBuffer_m, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    if (m_pipeline.graphicsHandle == VK_NULL_HANDLE) {
        throw std::runtime_error("pipeline is Nulled");
    }
    std::cerr << "Try Bind Pipeline...\n";
    // 2. Привязываем наш скомпилированный графический конвейер со всеми шейдерами
    vkCmdBindPipeline(currentCommandBuffer_m, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.graphicsHandle);
    std::cerr << "Pipeline is Binded\n";
    
    // Матрица вида
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 3.0f),  // Положение камеры в мире
        glm::vec3(0.0f, 0.0f, 0.0f),  // Куда камера смотрит (центр сцены)
        glm::vec3(0.0f, -1.0f, 0.0f)   // Направление "вверх" для камеры
    );
    // Матрица перспективы
    Perspective_p p{};
    glm::mat4 proj = p.createPespective();
    // Биндим наш вершинный буфер
    VkBuffer vertexBuffers[] = { vertexBuffer_m };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(currentCommandBuffer_m, 0, 1, vertexBuffers, offsets); // Слот 0, 1 буфер
   
    // Биндим Индексный буфер 
    // Передаем буфер, смещение (0) и тип индексов. У нас uint16_t, поэтому VK_INDEX_TYPE_UINT16
    vkCmdBindIndexBuffer(currentCommandBuffer_m, m_indexBuffer, 0, VK_INDEX_TYPE_UINT16);


    size_t totalObjects = world.transformPool->size();
    TransformComponent* transforms = world.transformPool->data();
    
    for(int i = 0; i < totalObjects; ++i) {
        TransformComponent& currentTransform = transforms[i];

        glm::mat4 model = glm::mat4(1.0f);

        model = glm::translate(model, glm::vec3(currentTransform.position, 0.0f));
        
        model = glm::rotate(model, currentTransform.rotation, glm::vec3(0.0f, 1.0f, 0.0f));
        
        model = glm::scale(model, glm::vec3(currentTransform.scale, 1.0f));

        MeshPushConstants constants{ model, proj, view};  
    
        // 3. Отправляем матрицу прямо в регистры видеокарты
        vkCmdPushConstants(
            currentCommandBuffer_m,
            m_pipeline.layout,                 // Лейаут конвейера, который знает про Push Constants
            VK_SHADER_STAGE_VERTEX_BIT,         // Целевой шейдер
            0,                                  // Смещение
            sizeof(MeshPushConstants),          // Сколько байт передать (192) 3 матрицы по 64 байт
            &constants                        // Указатель на данные на CPU
        );
   
        // 3. САМАЯ ГЛАВНАЯ КОМАНДА: Рисуем! 
        // Параметры: 3 вершины, 1 инстанс, первая вершина с индексом 0, первый инстанс 0.
        vkCmdDrawIndexed(currentCommandBuffer_m, 6, 1, 0, 0, 0); 
    }
    
    //std::cerr << "This?\n\n";
    //imgui.Render(currentCommandBuffer_m);
    //std::cerr << "IMGUI render is successfully\n";
    // Конец прохода рендеринга
    vkCmdEndRenderPass(currentCommandBuffer_m);


    // ==== 2. ОТДЕЛЬНЫЙ ПРОХОД ДЛЯ IMGUI ====
    // Копируем настройки прохода
    VkRenderPassBeginInfo imguiRenderPassInfo = renderPassInfo;
    
    // Зануляем Clear-значения, чтобы повторный вызов Render Pass не стёр треугольник
    //imguiRenderPassInfo.clearValueCount = 0; 
    //imguiRenderPassInfo.pClearValues = nullptr;
    // Открываем точно такой же проход второй раз, но уже с чистым графическим стейтом
    vkCmdBeginRenderPass(currentCommandBuffer_m, &imguiRenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        
    std::cerr << "This?\n\n";
    imgui.Render(currentCommandBuffer_m); 
    std::cerr << "IMGUI render is successfully\n";

    // Закрываем проход ImGui
    vkCmdEndRenderPass(currentCommandBuffer_m); 

    // Завершаем запись буфера
    if (vkEndCommandBuffer(currentCommandBuffer_m) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}
*/

bool VulkanContext::init_vulkan_core(Window& window_vulkan) { 
    
    // 1. Создаем Инстанс для инициализации и контекста 
    if (!m_instance.Init(enableValidationLayers, 
        window_vulkan.get_ExtentionCount(),
        window_vulkan.get_glfwExtention())) { 
        LOG_ERROR("[ERROR] Failed to init Insatnce");
        return false;
    }
    
    // 2. Создаем Поверхность (Surface) для связи Vulkan и окна ОС
    if (!m_surface.Init(m_instance.get_instance(), window_vulkan.get_window())) {
        LOG_ERROR("[ERROR] Failed to init Surface!");
        return false;
    }

    // 3. Запрашиваем физическое устройство и Создаем логическое устройство
    if (!m_device.Init(m_instance.get_instance(), m_surface.get_surface())) {
        LOG_ERROR("[ERROR] Failed to init Device!");
        return false;
    }

    // 4. Создаем Swapchain и с его помощью imageView
    if (!m_swapchain.init(m_device.get_physical_device(), m_device.get_device(), m_surface.get_surface(), window_vulkan.get_width(), window_vulkan.get_height())) {
        LOG_ERROR("[ERROR] Failed to init Swapchain!");
        return false;
    }

    LOG("Init geometry Render Pass...");
    // 5. Создаем Рендер пасс для геометрии
    if (!m_geometryRenderPass.Init(m_device.get_device(), m_swapchain.get_surfaceFormat().format, RenderPassType::Geometry3D)) { 
        LOG_ERROR("[ERROR] Failed to init Geometry Render Pass!");
        return false;
    }

    LOG("Init Gui Render Pass...");
    // 6. Создаем Рендер пасс для интерфейса(ImGui)
    if (!m_imGuiRenderPass.Init(m_device.get_device(), m_swapchain.get_surfaceFormat().format, RenderPassType::ImGuiUI)) {
        LOG_ERROR("[ERROR] Failed to init ImGui Render Pass!");
        return false;
    }
    
    LOG("Creating Frame Buffers..");
    // 7. Создаем ФрэймБуфер на основе созданного выше рендер пасса
    m_swapchain.createFramebuffers(m_geometryRenderPass.get_render_pass());
   

    LOG("Init pipeline...");
    // 8. Создаем Пайплайн 
    // Внимание: Нужно заранее скомпилировать шейдеры в байткод(SPIR-V)
    if (!m_pipeline.Init(m_device.get_device(), m_geometryRenderPass.get_render_pass(), "shaders/vert.spv", "shaders/frag.spv")) {
        LOG_ERROR("[ERROR] Failed to init Pipeline!");
        return false;
    }
    LOG("Init comands and synchronization...");
    // 9. Инициализация команд и синхронизации
    if (!init_commands_and_sync()) {
        LOG_ERROR("[ERROR] Failed to init Commands and Sync Objects!");
        return false;
    }

    LOG("[LOG] Vulkan Core fully initialized. Starting Game Loop...");
    return true;    
}


//void VulkanContext::draw_frame(const World& world, GuiLayer& imgui, uint32_t index) {
//    // 1. Запрашиваем индекс следующего доступного изображения из Swapchain
//    uint32_t imageIndex;
//    VkSemaphore imageAvailableSemaphore = m_frames[index].imageAvailableSemaphore;
//    VkSemaphore renderFinishedSemaphore = m_frames[index].renderFinishedSemaphore;
//    vkAcquireNextImageKHR(m_device.get_device(), m_swapchain.get_swapchain(), UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
//
//    // Перезаписываем командный буфер под фреймбуфер текущего кадра
//    vkResetCommandBuffer(currentCommandBuffer_m, 0);
//    //recordCommandBuffer(world, imageIndex, imgui);
//    // 2. Отправляем буфер команд на выполнение в графическую очередь (GPU)
//    VkSubmitInfo submitInfo{};
//    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//
//    // Ждем, пока картинка освободится свопчейном
//    VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
//    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
//    submitInfo.waitSemaphoreCount = 1;
//    submitInfo.pWaitSemaphores = waitSemaphores;
//    submitInfo.pWaitDstStageMask = waitStages;
//
//    submitInfo.commandBufferCount = 1;
//    submitInfo.pCommandBuffers = &currentCommandBuffer_m;
//
//    // Сигнализируем этот семафор, когда GPU закончит рендер
//    VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
//    submitInfo.signalSemaphoreCount = 1;
//    submitInfo.pSignalSemaphores = signalSemaphores;
//
//    if (vkQueueSubmit(m_device.get_graphics_queue(), 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
//        LOG_ERROR("[ERROR] Failed to submit draw command buffer!");
//        return;
//    }
//
//    // 3. Отправляем готовый кадр на экран (Презентация)
//    VkPresentInfoKHR presentInfo{};
//    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
//    presentInfo.waitSemaphoreCount = 1;
//    presentInfo.pWaitSemaphores = signalSemaphores; // Ждем окончания рендера перед выводом
//
//    VkSwapchainKHR swapChains[] = {m_swapchain.get_swapchain()};
//    presentInfo.swapchainCount = 1;
//    presentInfo.pSwapchains = swapChains;
//    presentInfo.pImageIndices = &imageIndex;
//
//    vkQueuePresentKHR(m_device.get_present_queue(), &presentInfo);
//
//    // Ждем, пока видеокарта полностью закончит операцию, чтобы не перегружать память процессора
//    // (Для полноценного DOD это грубо, но для первого треугольника — идеально стабильно)
//    vkDeviceWaitIdle(m_device.get_device());
//}


void VulkanContext::cleanup() {
    VkDevice device = m_device.get_device();
 
    // Защита: если устройство вообще не было создано, мы не можем вызывать vkDeviceWaitIdle
    if (device != VK_NULL_HANDLE) {
        // 1. Обязательно ждем, пока GPU закончит все операции кадра
        vkDeviceWaitIdle(device);

        // 2. Уничтожаем пулы команд и объекты синхронизации (Frames in Flight)
        // Они управляют отправкой команд на устройство, поэтому гасим их первыми
        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            if (m_frames[i].inFlightFence) {
                vkDestroyFence(device, m_frames[i].inFlightFence, nullptr);
                m_frames[i].inFlightFence = VK_NULL_HANDLE;
            }
            if (m_frames[i].imageAvailableSemaphore) {
                vkDestroySemaphore(device, m_frames[i].imageAvailableSemaphore, nullptr);
                m_frames[i].imageAvailableSemaphore = VK_NULL_HANDLE;
            }
            if (m_frames[i].renderFinishedSemaphore) {
                vkDestroySemaphore(device, m_frames[i].renderFinishedSemaphore, nullptr);
                m_frames[i].renderFinishedSemaphore = VK_NULL_HANDLE;
            }
            if (m_frames[i].commandPool) {
                // Выделенные command buffers удалятся автоматически вместе с пулом
                vkDestroyCommandPool(device, m_frames[i].commandPool, nullptr);
                m_frames[i].commandPool = VK_NULL_HANDLE;
            }
        }

        // 3. Уничтожаем ресурсы геометрии (выделенную память и буферы)
        if (vertexBuffer_m != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, vertexBuffer_m, nullptr);
            vertexBuffer_m = VK_NULL_HANDLE;
        }
        if (vertexBufferMemory_m != VK_NULL_HANDLE) {
            vkFreeMemory(device, vertexBufferMemory_m, nullptr);
            vertexBufferMemory_m = VK_NULL_HANDLE;
        }

        // 4. Уничтожаем пайплайн (он жестко завязан на форматы рендер-пасса)
        m_pipeline.cleanup();

        // 5. Уничтожаем проходы рендеринга
        m_imGuiRenderPass.cleanup();
        m_geometryRenderPass.cleanup();

        // 6. Уничтожаем Swapchain (внутри его cleanup() должны удаляться VkImageView и VkFramebuffer!)
        m_swapchain.cleanup();

        // 7. Уничтожаем само логическое устройство (VkDevice)
        // Теперь это делать на 100% безопасно, так как все дочерние объекты мертвы
        m_device.cleanup();
    }

    // 8. Уничтожаем базовые модули инициализации (им не нужен VkDevice)
    // Поверхность зависит от инстанса, поэтому сначала гасим её
    m_surface.cleanup(); 
    m_instance.cleanup();
}


uint32_t VulkanContext::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_device.get_physical_device(), &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    LOG_ERROR("Failed to find suitable memory type!");
}

bool VulkanContext::createVertexBuffer(const Vertex* vertices) {
    
    VkDeviceSize bufferSize = sizeof(vertices);

    // Создаем сам буфер
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT; // Говорим, что это вершинный буфер
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_device.get_device(), &bufferInfo, nullptr, &vertexBuffer_m) != VK_SUCCESS) {
        LOG_ERROR("Failed to create buffer");
        return false;
    }

    // Запрашиваем требования к памяти для этого буфера
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device.get_device(), vertexBuffer_m, &memRequirements);

    // Выделяем память на GPU (для тестов используем Host Visible память, которую процессор может маппить)
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(m_device.get_device(), &allocInfo, nullptr, &vertexBufferMemory_m) != VK_SUCCESS) {
        LOG_ERROR("Failed to allocate memory");
        return false;
    }

    // Связываем буфер и выделенную память
    vkBindBufferMemory(m_device.get_device(), vertexBuffer_m, vertexBufferMemory_m, 0);

    // Копируем данные из вектора C++ прямо в память видеокарты (Маппинг)
    void* data;
    vkMapMemory(m_device.get_device(), vertexBufferMemory_m, 0, bufferSize, 0, &data);
    memcpy(data, vertices, (size_t)bufferSize);
    vkUnmapMemory(m_device.get_device(), vertexBufferMemory_m);
}
/*
void VulkanContext::createIndexBuffer(const std::vector<uint16_t> indices) { 

    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size(); // Размер в байтах (6 * 2 байта = 12 байт)

    // 1. Создаем буфер
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT; // Внимание: ИНДЕКСНЫЙ БУФЕР!
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device_m, &bufferInfo, nullptr, &m_indexBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create index buffer!");
    }

    // 2. Запрашиваем требования к памяти
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device_m, m_indexBuffer, &memRequirements);

    // 3. Выделяем память на GPU (используем тот же твой findMemoryType)
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(device_m, &allocInfo, nullptr, &m_indexBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate index buffer memory!");
    }

    // 4. Связываем память и буфер
    vkBindBufferMemory(device_m, m_indexBuffer, m_indexBufferMemory, 0);

    // 5. Открываем портал (Маппинг) и заливаем индексы на GPU
    void* data;
    vkMapMemory(device_m, m_indexBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t)bufferSize);
    vkUnmapMemory(device_m, m_indexBufferMemory);
}
*/


bool VulkanContext::init_commands_and_sync() {
    VkDevice device = m_device.get_device();
    
    // Нам нужен индекс графического семейства для создания Command Pool
    // Мы можем пересчитать его или вытащить из m_device, если ты сохранил его там.
    // Предположим, у твоего m_device есть метод получения индексов очередей.
    uint32_t graphicsFamilyIndex = m_device.get_graphic_family_indices();

    // Заполняем структуры настроек на стеке ОДИН РАЗ, так как для всех кадров они одинаковы
    
    // Настройки пула команд
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = graphicsFamilyIndex;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Разрешаем сброс буфера каждый кадр

    // Настройки буфера команд
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    // Настройки семафоров
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // Настройки фенса
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    // КРИТИЧЕСКИ ВАЖНЫЙ ФЛАГ: создаем фенс СРАЗУ в сигнальном состоянии.
    // Если этого не сделать, при самом первом кадре игра зависнет, ожидая пока GPU что-то сделает.
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; 

    // А теперь просто в цикле создаем ресурсы для каждого "кадра в полете"
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        
        // 1. Создаем персональный Пул Команд для кадра
        if (vkCreateCommandPool(device, &poolInfo, nullptr, &m_frames[i].commandPool) != VK_SUCCESS) {
            LOG_ERROR("[ERROR] Failed to create Command Pool for frame!");
            return false;
        }

        // 2. Выделяем персональный Буфер Команд из этого пула
        allocInfo.commandPool = m_frames[i].commandPool; // Подставляем пул текущего кадра
        if (vkAllocateCommandBuffers(device, &allocInfo, &m_frames[i].commandBuffer) != VK_SUCCESS) {
            LOG_ERROR("[ERROR] Failed to allocate Command Buffer for frame!");
            return false;
        }

        // 3. Создаем семафоры
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_frames[i].imageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_frames[i].renderFinishedSemaphore) != VK_SUCCESS) {
            LOG_ERROR("[ERROR] Failed to create Semaphores for frame!");
            return false;
        }

        // 4. Создаем фенс
        if (vkCreateFence(device, &fenceInfo, nullptr, &m_frames[i].inFlightFence) != VK_SUCCESS) {
            LOG_ERROR("[ERROR] Failed to create Fence for frame!");
            return false;
        }
    }

    LOG("[LOG] Command pools, buffers, and sync objects for 2 frames initialized successfully!");
    return true;
}

