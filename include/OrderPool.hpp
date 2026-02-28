#pragma once

#include "types.hpp"

#include <cstddef>

class OrderPool 
{
public:
    explicit OrderPool(size_t capacity);
    ~OrderPool();

    OrderPool(const OrderPool& other) = delete;
    OrderPool& operator=(const OrderPool& other) = delete;

    Order* allocate();
    void deallocate(Order* order);

    size_t capacity() const { return capacity_; };
    size_t available() const { return free_count_; }
private:
    Order* storage_; // raw array
    Order* free_list_; // head of free list
    size_t capacity_;
    size_t free_count_;
};
