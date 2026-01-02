#pragma once
#include "book_side.hpp"
#include "market_event.hpp"

constexpr size_t MAX_PRICE_LEVELS = 4096;

class OrderBook {
public:
        OrderBook(Price bid_base, Price ask_base)
                : bids(bid_base), asks(ask_base) {}

        inline void on_event(const MarketEvent& ev) noexcept {
		BookSide<MAX_PRICE_LEVELS>& side = ev.is_bid ? bids : asks;

		switch (ev.type) {
			case EventType::Add:
				side.add(ev.price, ev.qty);
				break;
			case EventType::Cancel:
				side.cancel(ev.price, ev.qty);
				break;
			case EventType::Trade:
				side.trade(ev.price, ev.qty);
				break;
		}
	}

        [[nodiscard]] inline Price best_bid() const noexcept { return bids.best(); }
        [[nodiscard]] inline Price best_ask() const noexcept { return asks.best(); }

private:
        BookSide<MAX_PRICE_LEVELS> bids;
        BookSide<MAX_PRICE_LEVELS> asks;
};
