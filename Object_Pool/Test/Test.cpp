#include <iostream>
#include "./../easy_Object_Pool/easy_Object_Pool.h"

using namespace std;

struct AT {
    AT(){}
    AT(int a, int b) : m_a(a), m_b(b){}

    void Fun() {
        cout << m_a << " " << m_b << endl;
    }

    int m_a = 0;
    int m_b = 0;
};

struct BT {
    void Fun() {
        cout << "from object b " << endl;
    }
};

void TestObjectPool() {
    ObjectPool<AT> pool;
    {
        auto p1 = pool.GetObject();
        p1 -> Fun();
    }
    
    auto p2 = pool.GetObject();
    p2->Fun();

    auto p3 = pool.GetObject();
    p3->Fun();

    // 此时会发现，我们很难去调用 AT(int a, int b) 构造函数
    // 所以需要对此进行优化
    // int a = 5, b = 6;
    // auto p4 = pool.GetObject(a, b);
    // p4->Fun();

    // auto p5 = pool.GetObject(3, 4);
    // p5->Fun();

    // {
    //     auto p6 = pool.GetObject(3, 4);
    //     p6->Fun();
    // }
    // auto p7 = pool.GetObject(7, 8);
    // p7->Fun();
}