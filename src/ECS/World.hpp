#pragma once
#include <memory>
#include "TransformComponents.hpp"
#include "Components.hpp"
class World {
public:
    World() = default;

    // Запрещаем копирование всего игрового мира
    World(const World&) = delete;
    World& operator=(const World&) = delete;

    std::unique_ptr<SparseSet<TransformComponent>> transformPool = std::make_unique<SparseSet<TransformComponent>>();
};
