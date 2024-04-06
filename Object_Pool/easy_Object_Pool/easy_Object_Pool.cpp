#include "easy_Object_Pool.h"



template<typename Object>
ObjectPool<Object>::ObjectPool(size_t unSize = SIZE) : m_unSize(unSize) {
    for (size_t unIdx = 0; unIdx < m_unSize; ++unIdx) {
        m_oPool.push_back(new Object());
    }
}

template<typename Object>
ObjectPool<Object>::~ObjectPool() {
    // typename std::list<Object *>::iterator oIt = m_oPool.begin();
    auto oIt = m_oPool.begin();
    while (oIt != m_oPool.end()) {
        delete (*oIt);
        ++oIt;
    }
    m_unSize = 0;
}

template<typename Object>
Object* ObjectPool<Object>::GetObject() {
    Object* pObj = nullptr;
    if (0 == m_unSize) {
        pObj = new Object();
    } else {
        pObj = m_oPool.front();
        m_oPool.pop_front();
        --m_unSize;
    }
    return pObj;
}
 
template<typename Object>
void ObjectPool<Object>::ReturnObject(Object * pObj) {
    m_oPool.push_back(pObj);
    ++m_unSize;
}