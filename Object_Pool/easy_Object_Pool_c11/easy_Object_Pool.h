#include <string>
#include <functional>
#include <tuple>
#include <map>

#include "./../Any.cpp"

using namespace std;
const int MaxObjectNum = 10;


class ObjectPool {

    template<typename T, typename... Args>
    using Constructor = function<shared_ptr<T>(Args...)>;

public:

    ObjectPool();
    ~ObjectPool();

    template<typename T, typename... Args>
    void Create(int num);        //默认创建多少个对象

    template<typename T, typename... Args>
    shared_ptr<T> createPtr(string& constructName, Args... args);


    template<typename T, typename... Args>
    shared_ptr<T> Get(Args... args);

private:
    template<typename T, typename... Args>
    shared_ptr<T> CreateInstance(Any& any, string& constructName, Args... args);

    template<typename T, typename... Args>
    void InitPool(T& f, string& constructName, Args... args);

    template<typename T, typename... Args>
    shared_ptr<T> GetInstance(string& constructName, Args... args);

private:
    multimap<string, Any> m_map;
    multimap<string, Any> m_object_map;
    map<string, int> m_counter;
    bool needClear;
};