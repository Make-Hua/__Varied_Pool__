#include <iostream>

#include "./../easy_Object_Pool_c11/easy_Object_Pool.cpp"

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
