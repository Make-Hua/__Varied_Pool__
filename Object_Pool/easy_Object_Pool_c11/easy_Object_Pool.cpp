#include "easy_Object_Pool.h"


ObjectPool::ObjectPool() {
    needClear = false;
};

ObjectPool::~ObjectPool() {
    needClear = true;
}

template<typename T, typename... Args>
void ObjectPool::Create(int num) {
    if (num <= 0 || num > MaxObjectNum)
        throw std::logic_error("object num errer");

    auto constructName = typeid(Constructor<T, Args...>).name();
    Constructor<T, Args...> f = [constructName, this](Args... args) {
        return createPtr<T>(string(constructName), args...);
    };
    m_map.emplace(typeid(T).name(), f);

    m_counter.emplace(constructName, num);
}

template<typename T, typename... Args>
shared_ptr<T> ObjectPool::createPtr(string& constructName, Args... args) {
    return shared_ptr<T>(new T(args...), [constructName, this](T* t) {
        if (needClear)
            delete[] t;
        else
            m_object_map.emplace(constructName, shared_ptr<T>(t));
    });
}


template<typename T, typename... Args>
shared_ptr<T> ObjectPool::Get(Args... args) {
    using ConstructType = Constructor<T, Args...>;

    string constructName = typeid(ConstructType).name();
    auto range = m_map.equal_range(typeid(T).name());

    for (auto it = range.first; it != range.second; ++it) {
        if (it->second.Is<ConstructType>()) {
            auto ptr = GetInstance<T>(constructName, args...);

            if (ptr != nullptr)
                return ptr;

            return CreateInstance<T, Args...>(it->second, constructName, args...);
        }
    }

    return nullptr;
}

template<typename T, typename... Args>
shared_ptr<T> ObjectPool::CreateInstance(Any& any, string& constructName, Args... args) {
    using ConstructType = Constructor<T, Args...>;
    ConstructType f = any.AnyCast<ConstructType>();
    return createPtr<T, Args...>(constructName, args...);
}


template<typename T, typename... Args>
void ObjectPool::InitPool(T& f, string& constructName, Args... args) {
    int num = m_counter[constructName];
    if (num != 0) {
        for (int i = 0; i < num - 1; i++) {
            m_object_map.emplace(constructName, f(args...));
        }
        m_counter[constructName] = 0;
    }
}



template<typename T, typename... Args>
shared_ptr<T> ObjectPool::GetInstance(string& constructName, Args... args) {
    auto it = m_object_map.find(constructName);
    if (it == m_object_map.end())
        return nullptr;

    auto ptr = it->second.AnyCast<shared_ptr<T>>();
    if (sizeof...(Args)>0)
        *ptr.get() = move(T(args...));

    m_object_map.erase(it);
    return ptr;
}
