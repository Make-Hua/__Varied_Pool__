// https://zhuanlan.zhihu.com/p/477802980
#include <stdlib.h>
#include <wtypes.h>
 
#define  MEMPOOL_ALIGNMENT 8            //对齐长度
//内存块，每个内存块管理一大块内存，包括许多分配单元
class MemoryBlock {
public:
    MemoryBlock (int nUnitSize,int nUnitAmount);
    ~MemoryBlock(){};
    static void*    operator new    (size_t, int nUnitSize, int nUnitAmount);
    static void     operator delete (void*, int nUnitSize, int nUnitAmount){};
    static void     operator delete (void* pBlock);
 
    int             nSize;              //该内存块的大小，以字节为单位
    int             nFree;              //该内存块还有多少可分配的单元
    int             nFirst;             //当前可用单元的序号，从0开始
    MemoryBlock*    pNext;              //指向下一个内存块
    char            aData[1];           //用于标记分配单元开始的位置，分配单元从aData的位置开始
     
};
 
class MemoryPool
{
public:
                    MemoryPool (int _nUnitSize,
                                int _nGrowSize = 1024,
                                int _nInitSzie = 256);    // 构造函数
                    ~MemoryPool();                        // 析构函数
    void*           Alloc();                              // 
    void            Free(void* pFree);
 
private:
    int             nInitSize;          //初始大小
    int             nGrowSize;          //增长大小
    int             nUnitSize;          //分配单元大小
    MemoryBlock*    pBlock;             //内存块链表
};