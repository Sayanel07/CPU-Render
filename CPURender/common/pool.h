#pragma once
#include <queue>
#include <vector>
#include "types.h"

template<class T>
class Pool
{
public:
    T& operator[](uint32 id) {
        return m_data[id];
    }
    uint32 Add(const T& object);
    uint32 Add(T&& object);
    void Remove(T object);
    void Remove(uint32 id);
private:
    std::vector<T> m_data;
    std::queue<uint32> m_ids;
};

template <class T>
uint32 Pool<T>::Add(const T& object) {
    if (m_ids.empty()) {
        m_data.push_back(object);
       return static_cast<uint32>(m_data.size() - 1);
    }
    uint32 id = m_ids.front();
    m_ids.pop();
    m_data[id] = object;
    return id;
}

template <class T>
uint32 Pool<T>::Add(T&& object) {
    if (m_ids.empty()) {
        m_data.push_back(std::move(object));
        return static_cast<uint32>(m_data.size() - 1);
    }
    uint32 id = m_ids.front();
    m_ids.pop();
    m_data[id] = std::move(object);
    return id;
}

template <class T>
void Pool<T>::Remove(T object) {
    auto it = std::find(m_data.begin(), m_data.end(), object);
    uint32 id = std::distance(m_data.begin(), it);
    m_data[id] = {};
    m_ids.push(id);
}

template <class T>
void Pool<T>::Remove(uint32 id) {
    m_data[id] = T{};
    m_ids.push(id);
}
