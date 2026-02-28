#include "OrderPool.hpp"

#include <stdlib.h>

OrderPool::OrderPool(size_t capacity)
    : capacity_(capacity)
    , free_count_(capacity)
{
    storage_ = static_cast<Order*>(
        std::aligned_alloc(64, capacity*sizeof(Order))
    );

    free_list_ = &storage_[0];
    for(size_t i=0; i<capacity_-1; i++)
    {
        storage_[i].next = &storage_[i+1];
    }
    storage_[capacity_-1].next = nullptr;
}

OrderPool::~OrderPool()
{
    std::free(storage_);
}

Order* OrderPool::allocate()
{
    if(!free_list_) return nullptr; // TODO: need to handle if pool is exhausted

    Order* order = free_list_;
    free_list_ = order->next;
    free_count_--;

    order->next = nullptr;
    order->prev = nullptr;

    return order;
}

void OrderPool::deallocate(Order* order)
{
    order->next = free_list_;
    free_list_ = order;
    free_count_++;
}