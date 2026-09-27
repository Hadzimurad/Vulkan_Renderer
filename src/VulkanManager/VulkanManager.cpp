#include "VulkanManager.hpp"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include "Core/ImGuiLayer/ImGuiLayer.hpp"

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

void VulkanContext::fillInitGUIStruct(VulkanGUICreateInfo& GUIcreateInfo) {
    GUIcreateInfo.renderPass = m_pipeline.renderPass;
    GUIcreateInfo.instance = instance_m;
    GUIcreateInfo.device = device_m;
    GUIcreateInfo.physicalDevice = physicalDevice_m;
    GUIcreateInfo.imageCount = m_swapChain.maxImageCount;
    GUIcreateInfo.minImageCount = m_swapChain.minImageCount;
    GUIcreateInfo.queue = m_queue.graphicsQueue; 
    GUIcreateInfo.queueFamily = static_cast<uint32_t>(m_queue.indices.graphicsFamily);
}

QueueFamilyIndices VulkanContext::findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        // Проверяем поддержку графики
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
            indices.hasGraphics = true;
        }

        // Проверяем поддержку вывода на экран (презентации)
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
            indices.hasPresent = true;
        }

        if (indices.isComplete()) {
            break;
        }
        i++;
    }
    
    return indices;
}

SwapChainSupportDetails VulkanContext::querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
    SwapChainSupportDetails details;

    // 1. Получаем базовые возможности
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    // 2. Получаем поддерживаемые форматы
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    // 3. Получаем режимы вывода
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

// Выбираем формат пикселей. Идеально: BGRA 8-бит со стандартным цветовым пространством SRGB
VkSurfaceFormatKHR VulkanContext::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && 
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }
    return availableFormats[0]; // Если идеального нет, берем первый попавшийся
}

// Выбираем режим вывода. MAILBOX — это тройная буферизация (минимальный инпут-лаг, без разрывов)
VkPresentModeKHR VulkanContext::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode; // Возвращаем тройную буферизацию, если она есть
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR; // FIFO есть ВСЕГДА — это стандартный V-Sync
}

// Выбираем разрешение кадров. Обычно это просто размер нашего окна GLFW (800x600)
VkExtent2D VulkanContext::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        VkExtent2D actualExtent = {800, 600};
        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
        return actualExtent;
    }
}

bool VulkanContext::isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    // КРИТИЧЕСКИ ВАЖНО: Проверяем, что видеокарта поддерживает версию 1.2
    if (deviceProperties.apiVersion < VK_API_VERSION_1_2) {
        return false;
    }

    QueueFamilyIndices indices = findQueueFamilies(device, surface);

    // Для GTX 660 нам важно, чтобы это была дискретная видеокарта (DISCRETE_GPU)
    return indices.isComplete() && deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
}

// Проверка поддержки слоев валидации
bool VulkanContext::checkValidationLayerSupport() {
    uint32_t layerCount;
    // Получаем все доступные слои
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    // Массив в котором все доступные на GPU слои
    std::vector<VkLayerProperties> availableLayers(layerCount);
    // Тут он иницализируется названиями доступных слоев
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());


    // Ищем все нужные слои в validationLayers если хотя бы одного нет return false
    for (const char* layerName : validationLayers) {
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

std::vector<char> VulkanContext::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file: " + filename);
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
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

    // 2. Привязываем наш скомпилированный графический конвейер со всеми шейдерами
    vkCmdBindPipeline(currentCommandBuffer_m, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.graphicsHandle);

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
    imgui.Render(currentCommandBuffer_m);   
    // Конец прохода рендеринга
    vkCmdEndRenderPass(currentCommandBuffer_m);

    // Завершаем запись буфера
    if (vkEndCommandBuffer(currentCommandBuffer_m) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}


int VulkanContext::init_vulkan_core(Window& window_vulkan) { 

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "DOD Engine";
    appInfo.apiVersion = VK_API_VERSION_1_2; 

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    
    createInfo.enabledExtensionCount = window_vulkan.get_ExtentionCount();
    createInfo.ppEnabledExtensionNames = window_vulkan.get_glfwExtention();

    if (enableValidationLayers) {
        if (!checkValidationLayerSupport()) return -1;
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    
    
    VkInstance instance{};
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        std::cerr << "Failed to create instance!" << std::endl;
        return -1;
    }
    instance_m = instance;


    // 1. Создаем Поверхность (Surface) для связи Vulkan и окна ОС
    VkSurfaceKHR surface{};
    if (!window_vulkan.CreateWindowSurface(instance, &surface)) return -1; 
    surface_m = surface;

    // 2. Выбор Физического Устройства (Видеокарты)
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        std::cerr << "Failed to find GPUs with Vulkan support!" << std::endl;
        return -1;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (isDeviceSuitable(device, surface)) {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        std::cerr << "Failed to find a suitable GPU (GTX 660 or equivalent with Vulkan 1.2)!" << std::endl;
        return -1;
    }

    // Выведем имя нашей видеокарты, чтобы убедиться, что выбралась GTX 660
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    std::cout << "Selected GPU: " << deviceProperties.deviceName << std::endl;
    physicalDevice_m = physicalDevice;

    // 3. Создание Логического Устройства (VkDevice)
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { 
        static_cast<uint32_t>(indices.graphicsFamily),
        static_cast<uint32_t>(indices.presentFamily)
    };

    float queuePriority = 1.0f; // Приоритет очереди (от 0.0 до 1.0)
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // Описываем фичи геометрии, которые нам пригодятся (например, анизотропная фильтрация)
    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE; 

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME // Это макрос для строки "VK_KHR_swapchain"
    };

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    // ВАЖНЕЙШЕЕ ИСПРАВЛЕНИЕ: Включаем расширение Swapchain на уровне железа!
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

    // В современных версиях Vulkan устройство не требует слоев, но для старых драйверов укажем 
    deviceCreateInfo.enabledLayerCount = 0;
    deviceCreateInfo.ppEnabledLayerNames = nullptr;
    

    VkDevice device;
    if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS) {
        std::cerr << "Failed to create logical device!" << std::endl;
        return -1;
    }
    device_m = device;

    // Получаем хэндлы (указатели) на сами очереди, куда мы будем слать команды
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);

    m_queue.graphicsQueue = graphicsQueue;
    m_queue.presentQueue = presentQueue;

    std::cout << "[LOG 1] Queues fetched. Querying swapchain support..." << std::endl << std::flush;
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, surface);
    
    
    
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);
    m_swapChain.extent = extent;
    std::cout << "[LOG 2] Chosen Format: " << surfaceFormat.format << ", Mode: " << presentMode << std::endl << std::flush;

    // Зануляем количество картинок до минимума для теста стабильности
    m_swapChain.minImageCount = swapChainSupport.capabilities.minImageCount;

    m_swapChain.maxImageCount = swapChainSupport.capabilities.minImageCount + 1;
    std::cout << "[LOG 3] Requested Image Count: " << m_swapChain.maxImageCount << std::endl << std::flush;

    VkSwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.pNext = nullptr;
    swapchainCreateInfo.flags = 0;
    swapchainCreateInfo.surface = surface;
    swapchainCreateInfo.minImageCount = m_swapChain.maxImageCount; // Минимально возможная двойная буферизация
    swapchainCreateInfo.imageFormat = surfaceFormat.format; // Должно быть 50 (B8G8R8A8_SRGB)
    swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent = extent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.queueFamilyIndexCount = 0;
    swapchainCreateInfo.pQueueFamilyIndices = nullptr;

    swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR; // Мы проверили, тут честная 1
    
    // ИСПРАВЛЕНИЕ: Пробуем INHERIT вместо OPAQUE на случай конфликта с темами Windows
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; 
    
    swapchainCreateInfo.presentMode = presentMode;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    std::cout << "[LOG 4] Calling vkCreateSwapchainKHR with Hardcoded Extent (800x600)..." << std::endl << std::flush;
    
    VkSwapchainKHR swapChain; 
    if (vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapChain) != VK_SUCCESS) {
        std::cerr << "[ERROR] vkCreateSwapchainKHR failed" << std::endl << std::flush;
        return -1;
    }
    m_swapChain.handle = swapChain;

    

    std::cout << "[LOG 5] Swapchain created. Fetching images..." << std::endl << std::flush;
    uint32_t swapChainImageCount;
    vkGetSwapchainImagesKHR(device, swapChain, &swapChainImageCount, nullptr);
    std::vector<VkImage> swapChainImages(swapChainImageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &swapChainImageCount, swapChainImages.data());
    m_swapChain.images = swapChainImages;

    std::cout << "[LOG 6] Images fetched: " << swapChainImages.size() << ". Entering main loop..." << std::endl << std::flush;

    std::cout << "[LOG 6.1] Creating Image Views..." << std::endl << std::flush;

    // Вектор, где мы будем хранить хэндлы наших представлений
    std::vector<VkImageView> swapChainImageViews(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkImageViewCreateInfo viewCreateInfo{};
        viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.image = swapChainImages[i]; // Привязываем к конкретной картинке из свопчейна
        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // Трактуем как обычную 2D текстуру
        viewCreateInfo.format = surfaceFormat.format;    // Формат должен строго совпадать (наш 50-й формат)

        // Позволяет перенаправлять каналы цвета (например, сделать текстуру монохромной). Нам это не нужно.
        viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        
        // Описываем, какая часть изображения нам нужна (актуально для текстурных атласов и мип-маппинга)
        viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // Работаем с цветом
        viewCreateInfo.subresourceRange.baseMipLevel = 0;  // Мип-уровни не используем
        viewCreateInfo.subresourceRange.levelCount = 1;
        viewCreateInfo.subresourceRange.baseArrayLayer = 0; // Слои массива не используем
        viewCreateInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &viewCreateInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
            std::cerr << "[ERROR] Failed to create image view for index " << i << std::endl << std::flush;
            return -1;
        }

    }
    m_swapChain.imageViews = swapChainImageViews;
    
    std::cout << "[LOG 6.2] All Image Views created successfully!" << std::endl << std::flush;

    std::cout << "[LOG 6.3] Creating Render Pass..." << std::endl << std::flush;

    // Описываем наш единственный буфер цвета (картинку свопчейна)
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = surfaceFormat.format; // Формат пикселей (50)
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // Без сглаживания (MSAA)

    // Действие при старте прохода: ОЧИЩАТЬ буфер (VK_ATTACHMENT_LOAD_OP_CLEAR)
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    
    // Действие в конце прохода: СОХРАНЯТЬ пиксели в памяти (VK_ATTACHMENT_STORE_OP_STORE)
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; 

    // С буфером трафарета (stencil) мы ничего не делаем
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    // Vulkan нужно знать топологию памяти картинки до и после рендера:
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;     // До рендера нам плевать, что там было
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // После рендера картинка должна быть готова к выводу на экран

    // Ссылка на наше описание (под-проходы рендеринга используют индексы)
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0; // Наш colorAttachment будет под нулевым индексом
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Во время рендера используем оптимальный режим для цвета

    // Создаем под-проход (Subpass). У нас простая сцена, поэтому под-проход всего один
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // Это графический под-проход, а не вычислительный
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    // Собираем сам Render Pass
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VkRenderPass renderPass;
    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        std::cerr << "[ERROR] Failed to create Render Pass!" << std::endl << std::flush;
        return -1;
    }
    m_pipeline.renderPass = renderPass;
    std::cout << "[LOG 6.4] Render Pass created successfully!" << std::endl << std::flush;


    std::cout << "[LOG 6.5] Compiling Graphics Pipeline..." << std::endl << std::flush;

    // 1. Загружаем откомпилированные SPIR-V файлы
    // Внимание: укажи правильный относительный путь от места запуска .exe файла к .spv
    std::vector<char> vertShaderCode = readFile("../shaders/vert.spv");
    std::vector<char> fragShaderCode = readFile("../shaders/frag.spv");

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



