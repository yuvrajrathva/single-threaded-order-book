#pragma once
#include <array>
#include <iostream>
#include <ostream>

#include "types.hpp"

template <Side S, std::size_t MaxLevels>
class BookSide
{
private:
	Price base_price_;
	Price tick_size_;
	int best_level_idx_;
	static constexpr Side side_ = S;
	std::array<PriceLevel, MaxLevels> levels_{};

public:
	explicit BookSide(Price base, Price tick_size)
		: base_price_(base), tick_size_(tick_size), best_level_idx_(-1) {}

	void add_order(Order* order, Price price) noexcept
	{
		const int idx = price_to_index(price);
		if(!is_valid_index(idx)) return;

		PriceLevel& level = levels_[idx];

		order->next = nullptr;
		order->prev = level.tail;

		if(level.tail)
		{
			level.tail->next = order;
		}
		else
		{
			level.head = order;
		}
		level.tail = order;

		level.total_qty += order->quantity;
		level.order_count++;
		
		update_best(idx);
	}

	void remove_order(Order* order, Price price) noexcept
	{
		const int idx = price_to_index(price);
		if(!is_valid_index(idx)) return;

		PriceLevel& level = levels_[idx];

		if(order->prev)
		{
			order->prev->next = order->next;
		}
		else
		{
			level.head = order->next;
		}

		if(order->next)
		{
			order->next->prev = order->prev;
		}
		else
		{
			level.tail = order->prev;
		}

		level.total_qty -= order->quantity;
        level.order_count--;

		if (level.order_count == 0 && idx == best_level_idx_) {
            best_level_idx_ = find_next_best_index(idx);
        }
	}

	void modify_order(Order* order, Price price, Qty new_qty) noexcept
	{
		const int idx = price_to_index(price);
		if(!is_valid_index(idx)) return;

		PriceLevel& level = levels_[idx];

		// TODO: here we can add move this order after tail if more quantity added (price-time priority)
		level.total_qty = level.total_qty - order->quantity + new_qty;
        order->quantity = new_qty
	}

	[[nodiscard]] Price get_best_price() const noexcept
	{
		if (best_level_idx_ == -1) return INVALID_PRICE;
        return base_price_ + (best_level_idx_ * tick_size_);
	}

	[[nodiscard]] Qty get_best_qty() const noexcept
    {
        if (best_level_idx_ == -1) return 0;
        return levels_[best_level_idx_].total_qty;
    }

	[[nodiscard]] const PriceLevel& get_level(Price price) const noexcept
	{
		const int idx = price_to_index(price);
		static const PriceLevel empty_level{};
		if(!is_valid_index(idx)) return empty_level;
		return levels_[idx];
	}

	void get_depth(DepthLevel* out, size_t max_levels, size_t& actual_count) const noexcept
	{
		actual_count = 0;
		if(best_level_idx_ == -1) return;
		
		if constexpr(side_ == Side::BID)
		{
			for(int i=best_level_idx_; i>=0 && actual_count < max_levels; i--)
			{
				if(levels_[i].order_count > 0)
				{
					out[actual_count++] = {
						.price = base_price_ + (i * tick_size_),
						.qty = levels_[i].total_qty,
						.order_count = levels_[i].order_count
					};
				}
			}
		}
		else
		{
			for(int i=best_level_idx_; i<static_cast<int>(MaxLevels) && actual_count < max_levels; i++)
			{
				if(levels_[i].order_count > 0)
				{
					out[actual_count++] = {
						.price = base_price_ + (i * tick_size_),
						.qty = levels_[i].total_qty,
						.order_count = levels_[i].order_count
					};
				}
			}
		}

	}
private:
	[[nodiscard]] bool is_valid_index(int idx) const noexcept
	{
		return idx >= 0 && idx < static_cast<int>(MaxLevels);
	}

	[[nodiscard]] int price_to_index(Price price) const noexcept
	{
		if(price < base_price_) return -1;

		const Price delta = price - base_price_;

		if(delta % tick_size_ != 0) return -1;

		return static_cast<int>(delta / tick_size_);
	}

	void update_best(int new_idx)
	{
		if(best_level_idx_ == -1)
		{
			best_level_idx_ = new_idx;
			return;
		}

		if constexpr (side_ == Side::BID)
		{
			if(new_idx > best_level_idx_) best_level_idx_ = new_idx;
		}
		else
		{
			if(new_idx < best_level_idx_) best_level_idx_ = new_idx;
		}
	}

	int find_next_best_index(int idx) 
	{
		if constexpr (side_ == Side.BID)
		{
			for(int i=idx-1; i>=0; i--)
			{
				if(levels_[i].order_count > 0) return i;
			}
		}
		else
		{
			for(int i=idx+1; i<static_cast<int>(MaxLevels); i++)
			{
				if(levels_[i].order_count > 0) return i;
			}
		}
		return -1;
	}
};
