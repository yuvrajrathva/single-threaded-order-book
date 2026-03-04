#pragma once
#include "book_side.hpp"
#include "market_event.hpp"
#include "orderbook_listener.hpp"
#include "OrderPool.hpp"
#include "types.hpp"
#include "absl/container/flat_hash_map.h"

#include <vector>
#include <chrono>

class OrderBook
{
private:
	BookSide<Side::BID, MAX_PRICE_LEVELS> bids_;
	BookSide<Side::ASK, MAX_PRICE_LEVELS> asks_;

	absl::flat_hash_map<uint64_t, Order*> order_map_;
	OrderPool pool_;
	
	std::vector<IOrderBookListener*> listeners_;

public:
	OrderBook(Price bid_base, Price ask_base, Price tick_size, size_t order_pool_capacity)
		: bids_(bid_base, tick_size), 
		  asks_(ask_base, tick_size), 
		  pool_(order_pool_capacity) {}

	void add_order(uint64_t order_id, Price price, Qty qty, Side side) noexcept
	{
		Order* order = pool_.allocate();
		if(!order) return;

		order->order_id = order_id;
		order->quantity = qty;
		order->side = side;

		if(side == Side::BID)
		{
			bids_.add_order(order, price);
		}
		else
		{
			asks_.add_order(order, price);
		}

		order_map_[order_id] = order;
		notify_if_best_changed();
	}

	void add_listener(IOrderBookListener* listener) {
        listeners_.push_back(listener);
    }

	[[nodiscard]] inline Price best_bid() const noexcept { return bids_.get_best_price(); }
	[[nodiscard]] inline Price best_ask() const noexcept { return asks_.get_best_price(); }
	[[nodiscard]] Qty best_bid_qty() const noexcept { return bids_.get_best_qty(); }
    [[nodiscard]] Qty best_ask_qty() const noexcept { return asks_.get_best_qty(); }

private:
	Price last_best_bid_ = INVALID_PRICE;
	Price last_best_ask_ = INVALID_PRICE;
	
	void notify_if_best_changed()
	{
		Price current_bid = bids_.get_best_price();
		Price current_ask = asks_.get_best_price();

		if(current_bid != last_best_bid_ || current_ask != last_best_ask_)
		{
			uint64_t ts = get_timestamp_ns();
			TopOfBook update{
				.best_bid = current_bid,
				.best_ask = current_ask,
				.recv_timestamp_ns = ts,
				.update_timestamp_ns = ts
			};

			for(auto* listener : listeners_) listener->on_book_update(update);

			last_best_bid_ = current_bid;
			last_best_ask_ = current_ask;
		}
	}

	inline uint64_t get_timestamp_ns() const noexcept {
		return std::chrono::duration_cast<std::chrono::nanoseconds>(
			std::chrono::steady_clock::now().time_since_epoch()
		).count();
	}
};
