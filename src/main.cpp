#include "VulkanManager/VulkanManager.hpp"
#include <exception>
#include "ECS/TransformComponents.hpp"
#include <memory>
#include "ECS/World.hpp"
#include "ECS/Systems.hpp"
#include "imgui.h"
#include "Core/ImGuiLayer/ImGuiLayer.hpp"

int main() {
    try {
        Window window{}; 
        VulkanContext vulkan;
        if (!vulkan.init_vulkan_core(window)) return -1; 
        World world;
        GuiLayer imgui{};
        VulkanGUICreateInfo guiCreateInfo{};
        
        //Инициализация структуры для GUI(ImGui)
        vulkan.fillInitGUIStruct(guiCreateInfo);
        imgui.Init(window.get_window(), std::move(guiCreateInfo));
    
        // Наш плоский DOD-массив вершин треугольника на CPU
        const std::vector<Vertex> vertices = {
            {{0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},  // Верхняя вершина (Красная)
            {{0.5f, 0.5f, 0.0f},  {0.0f, 1.0f, 0.0f}},  // Правая вершина (Зеленая)
            {{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},   // Левая вершина (Синяя)
            {{-0.5f, -0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}}  // Верхняя вершина (Красная)
        };
        const std::vector<uint16_t> indices = {
            0, 1, 2, 
            2, 3, 0
        };

        vulkan.createVertexBuffer(std::move(vertices));
        vulkan.createIndexBuffer(std::move(indices));
        // Создаем две сущности (просто два ID)
        Entity triangleA = 0;
        // Заталкиваем их начальные трансформы в плотный массив
        TransformComponent transformA{glm::vec2(-0.7f, 0.0f), 0.0f, glm::vec2(0.5f, 0.5f)};
        world.transformPool->assign(triangleA, transformA);
        
        
        float lastFrameTime = 0.0f;

        while (!window.should_close()) {
            window.pollEvents();
            imgui.BeginFrame();

            ImGui::Begin("Редактор сцены");
            ImGui::Text("Привет! Движок успешно запущен на Vulkan 1.2.");
            static float lightPos[3] = { 0.0f, 5.0f, 0.0f };
            ImGui::DragFloat3("Позиция Света", lightPos, 0.1f);
            
            ImGui::End();

            imgui.EndFrame();
            

            float currentFrameTime = static_cast<float>(glfwGetTime());
            float deltaTime = currentFrameTime - lastFrameTime;
            lastFrameTime = currentFrameTime;

            Systems::MovementSystem(world, deltaTime);    

            vulkan.draw_frame(world, imgui);

        }
        vulkan.cleanup();
         
    } catch (const std::exception& e) {
        std::cerr << "\n[FATAL ERROR]: " << e.what() << std::endl;
        return -1;
    }
    return 0; 
}

