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

struct TopOfBook
{
    Price best_bid;
    Price best_ask;
    Qty best_bid_qty;
    Qty best_ask_qty;
    Price spread;
    uint64_t recv_timestamp_ns;   // When WebSocket received the message
    uint64_t update_timestamp_ns; // When orderbook was updated
};

struct DepthLevel
{
    Price price;
    Qty qty;
    uint32_t order_count;
};

struct MarketDepth
{
    std::array<DepthLevel, 5> bids;
    std::array<DepthLevel, 5> asks;
    size_t bid_levels;  // levels actually populated and other indexes are empty/dummy
    size_t ask_levels;
};