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

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

// void VulkanContext::fillInitGUIStruct(VulkanGUICreateInfo& GUIcreateInfo) {
//     GUIcreateInfo.renderPass = m_pipeline.renderPass;
//     GUIcreateInfo.instance = instance_m;
//     GUIcreateInfo.device = device_m;
//     GUIcreateInfo.physicalDevice = physicalDevice_m;
//     GUIcreateInfo.imageCount = m_swapChain.maxImageCount;
//     GUIcreateInfo.minImageCount = m_swapChain.minImageCount;
//     GUIcreateInfo.queue = m_queue.graphicsQueue; 
//     GUIcreateInfo.queueFamily = static_cast<uint32_t>(m_queue.indices.graphicsFamily);
// }






/**
 * Читает файл в заранее выделенный пользователем буфер.
 * Никаких std::string, std::vector или скрытых аллокаций.
 */
FileResult readFile(const char* filepath, char* outBuffer, size_t bufferSize, size_t* outBytesRead) {
    if (!filepath || !outBuffer || bufferSize == 0) {
        return FILE_ERROR_READ;
    }

    // Используем классический Си-I/O
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        return FILE_ERROR_OPEN;
    }

    // Определяем размер файла (стандартный для Orthodox C++ способ без внешних библиотек)
    // SEEK_END перетаскивает указатель файла в конец буфера
    fseek(file, 0, SEEK_END);
    // ftell возращает номер элемента под которым стоит наш указатель
    // так как мы передвинули его с помощью SEEK_END он вернет номер после последнего элемента
    long fileSize = ftell(file);
    // возращаем указатель обратно
    fseek(file, 0, SEEK_SET);

    if (fileSize < 0) {
        fclose(file);
        return FILE_ERROR_READ;
    }

    // Проверяем, поместится ли файл в буфер (учитываем +1 для нуль-терминатора, если это текст)
    if ((size_t)fileSize >= bufferSize) {
        fclose(file);
        return FILE_ERROR_BUFFER_TOO_SMALL;
    }

    // Прямое чтение в буфер fread возвращает кол-во успешно прочитанных элементов
    size_t bytesRead = fread(outBuffer, 1, (size_t)fileSize, file);
    fclose(file); // Явное закрытие, никакого RAII/деструкторов для файлов

    if (bytesRead != (size_t)fileSize) {
        return FILE_ERROR_READ;
    }

    // Гарантируем нуль-терминатор, если буфер используется как C-строка
    outBuffer[bytesRead] = '\0'; 

    if (outBytesRead) {
        *outBytesRead = bytesRead;
    }

    return FILE_SUCCESS;
}


VkShaderModule VulkanContext::createShaderModule(VkDevice device, const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}


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

    LOG("[LOG 1] Queues fetched. Querying swapchain support...\n");

    // 4. Создаем Swapchain и с его помощью imageView
    if (!m_swapchain.init(m_device.get_physical_device(), m_device.get_device(), m_surface.get_surface(), 800, 600)) {
        LOG_ERROR("[ERROR] Failed to init Swapchain!");
        return false;
    }

    LOG("[LOG 6.1] Creating Render Pass...");

    if (!m_geometryRenderPass.Init(m_device.get_device(), m_swapchain.get_surfaceFormat().format, RenderPassType::Geometry3D)) { 
        LOG("[ERROR] Failed to init Geometry Render Pass!");
        return false;
    }

    if (!m_imGuiRenderPass.Init(m_device.get_device(), m_swapchain.get_surfaceFormat().format, RenderPassType::ImGuiUI)) {
        LOG("[ERROR] Failed to init ImGui Render Pass!");
        return false;
    }

    m_swapchain.createFramebuffers(m_geometryRenderPass.get_render_pass());

    LOG("[LOG 6.4] Render Pass created successfully!");
    LOG("[LOG 6.5] Compiling Graphics Pipeline...");

    // 1. Загружаем откомпилированные SPIR-V файлы
    // Внимание: нужен правильный относительный путь от места запуска .exe файла к .spv
    char staticBufferVert[1024];
    size_t bytesReadVert = 0;
    FileResult resultVert = readFile("../shaders/vert.spv", staticBufferVert, sizeof(staticBufferVert), &bytesReadVert);
    char staticBufferFrag[1024];
    size_t bytesReadFrag = 0;
    FileResult resultFrag = readFile("../shaders/frag.spv", staticBufferFrag, sizeof(staticBufferFrag), &bytesReadFrag);

    VkShaderModule vertShaderModule = createShaderModule(device, vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(device, fragShaderCode);

    // Привязываем шейдерные модули к конкретным стадиям конвейера
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT; // Вершинная стадия
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main"; // Точка входа в шейдере

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT; // Фрагментная стадия
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // 2. Настройка Vertex Input (Ввод вершин)
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data(); // Передаем атрибуты



    // 3. Топология геометрии (Input Assembly)
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // Рисуем треугольники
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // 4. Настройка Viewport и Scissor (Окно вывода)
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = extent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // 5. Растеризатор (Rasterizer)
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // Заливка полигонов цветом
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;  // Отсекаем задние грани
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    // 6. Мультисэмплинг (Сглаживание) - пока отключаем
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // 7. Смешивание цветов (Color Blending)
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE; // Пока без прозрачности

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // Передаем в вершинный шейдер
    pushConstantRange.offset = 0;                             // Начинаем с 0-го байта
    pushConstantRange.size = sizeof(MeshPushConstants);        // Размер (64 байта)

    // 8. Pipeline Layout (Сюда в будущем пойдут дескрипторы DOD-матриц и камер)
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;



    VkPipelineLayout pipelineLayout;
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout");
    }
    m_pipeline.layout = pipelineLayout;

    // 9. Сборка самого Графического Конвейера!
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr; // Пока без теста глубины
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr;
    
    pipelineInfo.layout = pipelineLayout; // Привязываем лэйаут
    pipelineInfo.renderPass = renderPass; // Привязываем наш RenderPass
    pipelineInfo.subpass = 0;

    VkPipeline graphicsPipeline;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("[ERROR] Failed to create graphics pipeline!");
    }
    m_pipeline.graphicsHandle = graphicsPipeline;

    // Модули шейдеров больше не нужны после создания конвейера, чистим их
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);

    std::cout << "[LOG 6.6] Graphics Pipeline compiled successfully!" << std::endl << std::flush;

    std::cout << "[LOG 6.7] Creating Framebuffers..." << std::endl << std::flush;

    // Массив для хранения фреймбуферов (в стиле DOD — плоский вектор объектов)
    std::vector<VkFramebuffer> swapChainFramebuffers(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        // Каждому фреймбуферу нужна ссылка на соответствующий ImageView цвета
        VkImageView attachments[] = {
            swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass; // Привязываем к нашему проходу рендеринга
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = extent.width;   // Разрешение должно строго совпадать
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1; // Всегда 1 слои для обычных 3D/2D окон

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            std::cerr << "[ERROR] Failed to create framebuffer for index " << i << std::endl << std::flush;
            return -1;
        }
    }

    m_swapChain.framebuffers = swapChainFramebuffers;

    std::cout << "[LOG 6.8] All Framebuffers created successfully!" << std::endl;
    
    std::cout << "[LOG 6.9] Creating Command Pool and Allocating Buffers..." << std::endl;

    // 1. Создаем Пул Команд
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = indices.graphicsFamily; // Наше графическое семейство очередей (0)
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Позволяет перезаписывать буфер, если понадобится

    VkCommandPool commandPool;
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        std::cerr << "[ERROR] Failed to create Command Pool!" << std::endl << std::flush;
        return -1;
    }

    commandPool_m = commandPool;

    // 2. Выделяем Командный Буфер из Пула
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // Первичный буфер (может быть отправлен в очередь напрямую)
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
        std::cerr << "[ERROR] Failed to allocate Command Buffers!" << std::endl << std::flush;
        return -1;
    }
    currentCommandBuffer_m = commandBuffer;


    // Создаем семафоры для синхронизации кадра
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkSemaphore imageAvailableSemaphore; // Сигнализирует, что картинка получена из свопчейна
    VkSemaphore renderFinishedSemaphore; // Сигнализирует, что рендеринг в текстуру завершен

    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) {
        std::cerr << "[ERROR] Failed to create Semaphores!" << std::endl << std::flush;
        return -1;
    }

    m_sync.imageAvailable = imageAvailableSemaphore;
    m_sync.renderFinished = renderFinishedSemaphore;

    std::cout << "[LOG 6.10] Synchronization objects created. Starting Game Loop..." << std::endl << std::flush;
    return 1;
}


void VulkanContext::draw_frame(const World& world, GuiLayer& imgui) {
    // 1. Запрашиваем индекс следующего доступного изображения из Swapchain
    uint32_t imageIndex;
    vkAcquireNextImageKHR(device_m, m_swapChain.handle, UINT64_MAX, m_sync.imageAvailable, VK_NULL_HANDLE, &imageIndex);

    // Перезаписываем командный буфер под фреймбуфер текущего кадра
    vkResetCommandBuffer(currentCommandBuffer_m, 0);
    recordCommandBuffer(world, imageIndex, imgui);
    // 2. Отправляем буфер команд на выполнение в графическую очередь (GPU)
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    // Ждем, пока картинка освободится свопчейном
    VkSemaphore waitSemaphores[] = {m_sync.imageAvailable};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &currentCommandBuffer_m;

    // Сигнализируем этот семафор, когда GPU закончит рендер
    VkSemaphore signalSemaphores[] = {m_sync.renderFinished};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(m_queue.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        std::cerr << "[ERROR] Failed to submit draw command buffer!" << std::endl;
        return;
    }

    // 3. Отправляем готовый кадр на экран (Презентация)
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores; // Ждем окончания рендера перед выводом

    VkSwapchainKHR swapChains[] = {m_swapChain.handle};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(m_queue.presentQueue, &presentInfo);

    // Ждем, пока видеокарта полностью закончит операцию, чтобы не перегружать память процессора
    // (Для полноценного DOD это грубо, но для первого треугольника — идеально стабильно)
    vkDeviceWaitIdle(device_m);
}


void VulkanContext::cleanup() {
    std::cout << "[LOG 7] Exited main loop. Cleaning up..." << std::endl << std::flush;

    vkDeviceWaitIdle(device_m);

    // Удаляем семафоры

    vkDestroySemaphore(device_m, m_sync.renderFinished, nullptr);
    vkDestroySemaphore(device_m, m_sync.imageAvailable, nullptr);

    // Удаляем пул команд (автоматически уничтожит все выделенные буферы)
    vkDestroyCommandPool(device_m, commandPool_m, nullptr);

    for (auto framebuffer : m_swapChain.framebuffers) {
        vkDestroyFramebuffer(device_m, framebuffer, nullptr);
    }

    vkFreeMemory(device_m, vertexBufferMemory_m, nullptr);
    vkFreeMemory(device_m, m_indexBufferMemory, nullptr);

    vkDestroyBuffer(device_m, m_indexBuffer, nullptr);
    vkDestroyBuffer(device_m, vertexBuffer_m, nullptr);

    vkDestroyPipeline(device_m, m_pipeline.graphicsHandle, nullptr);
    vkDestroyPipelineLayout(device_m, m_pipeline.layout, nullptr);

    // 1. Уничтожаем Render Pass
    vkDestroyRenderPass(device_m, m_pipeline.renderPass, nullptr); 

    // 2. Уничтожаем все Image Views в цикле
    for (auto imageView : m_swapChain.imageViews) {
        vkDestroyImageView(device_m, imageView, nullptr);
    } 
    
    vkDestroySwapchainKHR(device_m, m_swapChain.handle, nullptr);
    vkDestroyDevice(device_m, nullptr);
    vkDestroySurfaceKHR(instance_m, surface_m, nullptr); // Поверхность уничтожается до инстанса
    vkDestroyInstance(instance_m, nullptr);
        
}


uint32_t VulkanContext::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_m, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("Failed to find suitable memory type!");
}

void VulkanContext::createVertexBuffer(const std::vector<Vertex> vertices) { 
    
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    // Создаем сам буфер
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT; // Говорим, что это вершинный буфер
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device_m, &bufferInfo, nullptr, &vertexBuffer_m) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create vertex buffer!");
    }

    // Запрашиваем требования к памяти для этого буфера
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device_m, vertexBuffer_m, &memRequirements);

    // Выделяем память на GPU (для тестов используем Host Visible память, которую процессор может маппить)
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(device_m, &allocInfo, nullptr, &vertexBufferMemory_m) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate vertex buffer memory!");
    }

    // Связываем буфер и выделенную память
    vkBindBufferMemory(device_m, vertexBuffer_m, vertexBufferMemory_m, 0);

    // Копируем данные из вектора C++ прямо в память видеокарты (Маппинг)
    void* data;
    vkMapMemory(device_m, vertexBufferMemory_m, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(device_m, vertexBufferMemory_m);
}

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



