#pragma once
#include <cstddef>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> 
// Компонент трансформации: только чистая математика
struct TransformComponent {
    glm::vec2 position{0.0f, 0.0f};
    float rotation{0.0f};
    glm::vec2 scale{1.0f, 1.0f};
};

struct MeshPushConstants {
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 proj = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
};

struct Perspective_p{
    float fov = glm::radians(45.0f);        // Угол обзора (Field of View)
    float aspect = 800.0f / 600.0f;        // Соотношение сторон экрана (ширина / высота)
    float nearPlane = 0.1f;                 // Ближняя плоскость отсечения
    float farPlane = 100.0f;

    inline glm::mat4 createPespective() {
        glm::mat4 p = glm::perspective(fov, aspect, nearPlane, farPlane);
        p[1][1] *= -1.0f; // <--- ИНВЕРСИЯ ОСИ Y ДЛЯ VULKAN 
        return p;
    }
};

// Компонент скорости (для физики)
struct VelocityComponent {
    glm::vec2 velocity{0.0f, 0.0f};
};

// Компонент-ссылка на меш (какой буфер рисовать)
struct MeshComponent {
    uint32_t vertexCount{0};
    // Здесь позже будет ID или указатель на ваш VkBuffer
};
