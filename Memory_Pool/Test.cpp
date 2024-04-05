#include <stdio.h>
#include "Memory_Pool.h"
 
class CTest
{
public:
                CTest(){data1 = data2 = 0;};
                ~CTest(){};
    void*       operator new (size_t);
    void        operator delete(void* pTest);
public:
 
    static      MemoryPool Pool;
    int         data1;
    int         data2;
};
 
void CTest::operator delete( void* pTest )
{  
    Pool.Free(pTest);  
}
 
 
void* CTest::operator new(size_t)
{
    return (CTest*)Pool.Alloc();
}
 
MemoryPool CTest::Pool(sizeof(CTest));
 
int main()
{
    CTest* pTest = new CTest;
    printf("%d",pTest->data2);
}