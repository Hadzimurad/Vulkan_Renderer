#pragma once
#include <memory>
#include "SpareSet.hpp"
#include "Components.hpp"


class World {
public:
    World() = default;

    // Запрещаем копирование всего игрового мира
    World(const World&) = delete;
    World& operator=(const World&) = delete;
    ISparseSet* spareSetsPool[128];
};
