#pragma once
#include "World.hpp"
#include "Components.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace Systems {

    // Чистая DOD-система движения и логики
    inline void MovementSystem(World& world, float deltaTime) {
        // Забираем указатель на плоский массив данных прямо из нашего шаблона
        size_t totalObjects = world.transformPool->size();
        TransformComponent* transforms = world.transformPool->data();

        for (size_t i = 0; i < totalObjects; ++i) {
            // Берем объект из кэша процессора
            TransformComponent& transform = transforms[i];

            // 1. Обновляем логику вращения на CPU!
            // Пусть четные объекты крутятся в одну сторону, нечетные — в другую
            float direction = (i % 2 == 0) ? 1.0f : -1.0f;
            
            // Увеличиваем угол поворота в компоненте
            transform.rotation += 1.0f * direction * deltaTime;

            // Если угол стал больше 360 градусов (2*PI), сбрасываем, чтобы не копить огромные числа
            if (transform.rotation > glm::two_pi<float>()) {
                transform.rotation -= glm::two_pi<float>();
            }
            if (transform.rotation < -glm::two_pi<float>()) {
                transform.rotation += glm::two_pi<float>();
            }

            // 2. Здесь же можно обновить позицию, если у тебя будет VelocityComponent:
            // transform.position += velocity[i].speed * deltaTime;
        }
    }

} // namespace Systems
