#pragma once
#include "market_data.hpp"
#include "order_book.hpp"

template <typename spsc>
class MarketDataPublisher {
private:
	spsc& queue;

public:
	explicit MarketDataPublisher(spsc& q)
		: queue(q) {}

	void publish(const OrderBook& book) noexcept {
		if(book.best_bid() != INVALID_PRICE && book.best_ask() != INVALID_PRICE) {
			TopOfBook tob {
				book.best_bid(),
				book.best_ask(),
				spread(book.best_bid(), book.best_ask())
			};

			queue.push(tob);
		}
	}

private:
	inline Price spread(Price best_bid, Price best_ask) const noexcept {
		return	best_ask - best_bid;
	}
};
