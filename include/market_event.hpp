#pragma once
#include "types.hpp"

enum class EventType : uint8_t {
        Add,
        Cancel,
        Trade
};

struct MarketEvent {
        EventType type;
        Price price;
        Qty qty;
        bool is_bid;
};
