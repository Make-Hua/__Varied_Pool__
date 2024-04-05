#include "Memory_Pool.h"
 
MemoryBlock::MemoryBlock( int nUnitSize,int nUnitAmount )
    :   nSize   (nUnitAmount * nUnitSize),
        nFree   (nUnitAmount - 1),  //构造的时候，就已将第一个单元分配出去了，所以减一
        nFirst  (1),                //同上
        pNext   (NULL) {
    //初始化数组链表，将每个分配单元的下一个分配单元的序号写在当前单元的前两个字节中
    char* pData = aData;
    //最后一个位置不用写入
    for( int i = 1; i < nSize - 1; i++)
    {
        (*(USHORT*)pData) = i;
        pData += nUnitSize;
    }
}
 
void* MemoryBlock::operator new(size_t, int nUnitSize,int nUnitAmount ) {
    return ::operator new( sizeof(MemoryBlock) + nUnitSize * nUnitAmount );
}
 
void MemoryBlock::operator delete( void* pBlock) {
    ::operator delete(pBlock);
}
 
MemoryPool::MemoryPool( int _nUnitSize, int _nGrowSize /*= 1024*/, int _nInitSzie /*= 256*/ ) {
    nInitSize = _nInitSzie;
    nGrowSize = _nGrowSize;
    pBlock = NULL;
    if(_nUnitSize > 4)
        nUnitSize = (_nUnitSize + (MEMPOOL_ALIGNMENT - 1)) & ~(MEMPOOL_ALIGNMENT - 1);
    else if( _nUnitSize < 2)
        nUnitSize = 2;
    else
        nUnitSize = 4;
}
 
MemoryPool::~MemoryPool() {
    MemoryBlock* pMyBlock = pBlock;
    while( pMyBlock != NULL)
    {
        pMyBlock = pMyBlock->pNext;
        delete(pMyBlock);
    }
}
 
void* MemoryPool::Alloc() {
    if( NULL == pBlock)
    {
        //首次生成MemoryBlock,new带参数，new了一个MemoryBlock类
        pBlock = (MemoryBlock*)new(nUnitSize,nInitSize) MemoryBlock(nUnitSize,nUnitSize);
        return (void*)pBlock->aData;
    }
 
    //找到符合条件的内存块
    MemoryBlock* pMyBlock = pBlock;
    while( pMyBlock != NULL && 0 == pMyBlock->nFree )
        pMyBlock = pMyBlock->pNext;
 
    if( pMyBlock != NULL)
    {
        //找到了，进行分配
        char* pFree = pMyBlock->aData + pMyBlock->nFirst * nUnitSize;
        pMyBlock->nFirst = *((USHORT*)pFree);
        pMyBlock->nFree--;
 
        return (void*)pFree;
    }
    else
    {
        //没有找到，说明原来的内存块都满了，要再次分配
 
        if( 0 == nGrowSize)
            return NULL;
         
        pMyBlock = (MemoryBlock*)new(nUnitSize,nGrowSize) MemoryBlock(nUnitSize,nGrowSize);
 
        if( NULL == pMyBlock)
            return NULL;
 
        //进行一次插入操作
        pMyBlock->pNext = pBlock;
        pBlock = pMyBlock;
 
        return (void*)pMyBlock->aData;
    }
}
 
void MemoryPool::Free( void* pFree ) {
    //找到p所在的内存块
    MemoryBlock* pMyBlock = pBlock;
    MemoryBlock* PreBlock = NULL;
    while ( pMyBlock != NULL && ( pBlock->aData > pFree || pMyBlock->aData + pMyBlock->nSize))
    {
        PreBlock = pMyBlock;
        pMyBlock = pMyBlock->pNext;
    }
 
    if( NULL != pMyBlock )      //该内存在本内存池中pMyBlock所指向的内存块中
    {      
        //Step1 修改数组链表
        *((USHORT*)pFree) = pMyBlock->nFirst;
        pMyBlock->nFirst  = (USHORT)((ULONG)pFree - (ULONG)pMyBlock->aData) / nUnitSize;
        pMyBlock->nFree++;
 
        //Step2 判断是否需要向OS释放内存
        if( pMyBlock->nSize == pMyBlock->nFree * nUnitSize )
        {
            //在链表中删除该block
             
            delete(pMyBlock);
        }
        else
        {
            //将该block插入到队首
            PreBlock = pMyBlock->pNext;
            pMyBlock->pNext = pBlock;
            pBlock = pMyBlock;
        }
    }
}