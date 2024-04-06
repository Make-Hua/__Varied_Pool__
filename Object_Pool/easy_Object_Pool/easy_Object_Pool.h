// https://www.cnblogs.com/qicosmos/p/3673723.html
#include <list>

using namespace std;

const int SIZE = 10;
 
template<typename Object>
class ObjectPool {
public:
    ObjectPool(size_t unSize = SIZE);
    ~ObjectPool();

    Object* GetObject();
    void ReturnObject(Object * pObj);
 
private:
    size_t m_unSize;
    std::list<Object *> m_oPool;
};