#include <string>
#include <functional>
#include <tuple>
#include <map>

#include "Any.cpp"

const int MaxObjectNum = 10;

class ObjectPool {
    template<typename T, typename... Args>
    using Constructor = std::function<std::shared_ptr<T>(Args...)>;
public:

    ObjectPool() : needClear(false) {
    }

    ~ObjectPool() {
        needClear = true;
    }

    //默认创建多少个对象
    template<typename T, typename... Args>
    void Create(int num) {
        if (num <= 0 || num > MaxObjectNum)
            throw std::logic_error("object num errer");

        auto constructName = typeid(Constructor<T, Args...>).name();

        Constructor<T, Args...> f = [constructName, this](Args... args)
        {
            return createPtr<T>(string(constructName), args...);
        };

        m_map.emplace(typeid(T).name(), f);

        m_counter.emplace(constructName, num);
    }

    template<typename T, typename... Args>
    std::shared_ptr<T> createPtr(std::string& constructName, Args... args) {
        return std::shared_ptr<T>(new T(args...), [constructName, this](T* t) {
            if (needClear)
                delete[] t;
            else
                m_object_map.emplace(constructName, std::shared_ptr<T>(t));
        });
    }

    template<typename T, typename... Args>
    std::shared_ptr<T> Get(Args... args) {
        using ConstructType = Constructor<T, Args...>;

        std::string constructName = typeid(ConstructType).name();
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

private:
    template<typename T, typename... Args>
    std::shared_ptr<T> CreateInstance(Any& any, std::string& constructName, Args... args) {
        using ConstructType = Constructor<T, Args...>;
        ConstructType f = any.AnyCast<ConstructType>();

        return createPtr<T, Args...>(constructName, args...);
    }

    template<typename T, typename... Args>
    void InitPool(T& f, std::string& constructName, Args... args) {
        int num = m_counter[constructName];

        if (num != 0) {
            for (int i = 0; i < num - 1; i++) {
                m_object_map.emplace(constructName, f(args...));
            }
            m_counter[constructName] = 0;
        }
    }

    template<typename T, typename... Args>
    std::shared_ptr<T> GetInstance(std::string& constructName, Args... args) {
        auto it = m_object_map.find(constructName);
        if (it == m_object_map.end())
            return nullptr;

        auto ptr = it->second.AnyCast<std::shared_ptr<T>>();
        if (sizeof...(Args)>0)
            *ptr.get() = std::move(T(args...));

        m_object_map.erase(it);
        return ptr;
    }

private:
    std::multimap<std::string, Any> m_map;
    std::multimap<std::string, Any> m_object_map;
    std::map<std::string, int> m_counter;
    bool needClear;
};