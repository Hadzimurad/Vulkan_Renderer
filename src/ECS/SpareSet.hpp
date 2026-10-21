#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdio.h>
#include "LogError/LogError.hpp"

using Entity = uint32_t;
constexpr Entity INVALID_INDEX = 0xFFFFFFFF;

// Нешаблонный базовый класс, чтобы хранить все пулы в одном массиве в World
class ISparseSet {
public:
    virtual ~ISparseSet() = default;
    virtual void remove(Entity entity) = 0;
    virtual bool has(Entity entity) const = 0;
    ISparseSet() = default;
};

template <typename T>
class SparseSet : public ISparseSet {
private:
    // Плотный массив без дыр хранит структуры компонентов
    T*       m_dense = nullptr;
    // Плотный массив хранит ID сущностей
    Entity*  m_dense_to_entity = nullptr;
    // Разреженный массив обычно бывает много дыр(INVALID_INDEX) 
    // хранит индекс массива m_dense_to_entity
    uint32_t*  m_sparse = nullptr;

    size_t   m_capacity = 0;
    size_t   m_size = 0;
    size_t   m_sparse_capacity = 0;
public:


    bool has(Entity entity) const override { 
        if (entity >= m_sparse_capacity) return false;
        return !(m_sparse[entity] == INVALID_INDEX); 
    }
    // В Orthodox C++ управление памятью явное. 
    // Вместо умных аллокаторов мы будем использовать realloc/malloc
    void reserve_dense(size_t new_capacity) {
        if (new_capacity <= m_capacity) return;

        // Перевыделяем память
        m_dense = static_cast<T*>(realloc(m_dense, new_capacity * sizeof(T)));
        m_dense_to_entity = static_cast<Entity*>(realloc(m_dense_to_entity, new_capacity * sizeof(Entity)));

        // Проверка Ассертов
        ASSERT(m_dense != nullptr, "[ERROR] reserve_dense is failed");
        ASSERT(m_dense_to_entity != nullptr, "[ERROR] reserve_dense: m_dense_to_entity allocation failed");

        // Сохраняем новую емкость
        m_capacity = new_capacity;
    }

    void reserve_sparse(size_t new_capacity) {
        if (new_capacity <= m_sparse_capacity) return;

        size_t old_capacity = m_sparse_capacity;

        // Перевыделение памяти
        m_sparse = static_cast<Entity*>(realloc(m_sparse, new_capacity * sizeof(Entity)));

        ASSERT(m_sparse != nullptr, "[ERROR] reserve_sparse failed");

        m_sparse_capacity = new_capacity;

        for(size_t i = old_capacity; i < new_capacity; ++i) {
            m_sparse[i] = INVALID_INDEX;
        }
    } 

    void assign(Entity entity, const T& component) {
        // Проверка наличия сущности
        if (has(entity)) return;

        // если ID сущности больше или равен емкости разреженного массива
        // значит, этот индекс вылетает за границы. Нужно расширять
        if (entity >= m_sparse_capacity) {
            // Выделяем память с запасом чтобы 
            // следующий большой ID не вызывал realloc повторно
            reserve_sparse(entity * 2 + 1);
        }

        // Проверяем, есть ли место в плотном массиве (m_dense)
        if (m_size >= m_capacity) {
            reserve_dense(m_capacity * 2);
        }
         
        // Записываем данные после того как убедились что свободная память есть
        size_t dense_index = m_size;
        m_sparse[entity] = static_cast<Entity>(dense_index);

        m_dense[dense_index] = component;
        m_dense_to_entity[dense_index] = entity;
        m_size++;
    }

    T& get(Entity entity) {
        ASSERT(has(entity), "[ERROR] Attemped to get non-existent component");
        return m_dense[m_sparse[entity]];
    }

    void remove(Entity entity) override {
        if (!has(entity)) return;

        size_t index_to_remove = m_sparse[entity];
        size_t last_dense_index = m_size - 1;

        Entity last_entity = m_dense_to_entity[last_dense_index];

        // Зеркальный своп
        m_dense[index_to_remove] = m_dense[last_dense_index];
        m_dense_to_entity[index_to_remove] = last_entity;

        m_sparse[last_entity] = index_to_remove;

        m_sparse[entity] = INVALID_INDEX;
        m_size--;
    }

    size_t size() const { return m_size; }
    T* data() { return m_dense; }

    SparseSet() {
        reserve_dense(8);
        reserve_sparse(64);
    }

    ~SparseSet() override {
        free(m_dense);
        free(m_dense_to_entity);
        free(m_sparse);
    }
};
