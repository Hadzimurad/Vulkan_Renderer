#pragma once
#include <vector>
#include <cstdint>
#include <iostream>

using Entity = uint32_t;
const Entity INVALID_INDEX = 0xFFFFFFFF;

template <typename T>
class SparseSet {
private:
    // Плотные массивы для данных любого типа T
    std::vector<T>      m_dense;
    std::vector<Entity> m_dense_to_entity;

    // Разреженный массив-зеркало
    std::vector<size_t> m_sparse;

public:
    SparseSet() = default;
    
    // Запрещаем копирование пула компонентов (они должны быть уникальны)
    SparseSet(const SparseSet&) = delete;
    SparseSet& operator=(const SparseSet&) = delete;

    bool has(Entity entity) const {
        if (entity >= m_sparse.size()) return false;
        return m_sparse[entity] != INVALID_INDEX;
    }

    void assign(Entity entity, const T& component) {
        if (has(entity)) return;

        if (entity >= m_sparse.size()) {
            m_sparse.resize(entity + 1, INVALID_INDEX);
        }

        size_t dense_index = m_dense.size();
        m_sparse[entity] = dense_index;

        m_dense.push_back(component);
        m_dense_to_entity.push_back(entity);
    }

    T& get(Entity entity) {
        return m_dense[m_sparse[entity]];
    }

    void remove(Entity entity) {
        if (!has(entity)) return;

        size_t index_to_remove = m_sparse[entity];
        size_t last_dense_index = m_dense.size() - 1;

        Entity last_entity = m_dense_to_entity[last_dense_index];

        // Зеркальный своп
        m_dense[index_to_remove] = m_dense[last_dense_index];
        m_dense_to_entity[index_to_remove] = last_entity;

        m_sparse[last_entity] = index_to_remove;

        m_dense.pop_back();
        m_dense_to_entity.pop_back();

        m_sparse[entity] = INVALID_INDEX;
    }

    size_t size() const { return m_dense.size(); }
    T* data() { return m_dense.data(); }
};

