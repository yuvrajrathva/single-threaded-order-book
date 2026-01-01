#pragma once
#include "market_data.hpp"
#include "order_book.hpp"

class MarketDataPublisher {
public:
	void publish(const OrderBook& book) noexcept;
};
