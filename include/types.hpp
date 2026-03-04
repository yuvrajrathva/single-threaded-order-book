#pragma once
#include <cstdint>

using Price = uint32_t;
using Qty = uint32_t;

constexpr Price INVALID_PRICE = static_cast<Price>(-1);

constexpr std::size_t MAX_PRICE_LEVELS = 1 << 17;

enum class Side 
{
    BID,
    ASK
};

struct Order 
{
    uint64_t order_id;
    Qty quantity;
    Side side;

    Order* next;
    Order* prev;
};

struct PriceLevel
{
	Qty total_qty = 0;
    uint32_t order_count = 0;
    Order* head = nullptr;
    Order* tail = nullptr;
};