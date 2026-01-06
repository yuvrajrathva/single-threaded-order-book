#pragma once
#include <array>
#include "types.hpp"

struct PriceLevel
{
	Qty volume = 0;
};

template <size_t MaxLevels>
class BookSide
{
private:
	Price base_price;
	Price best_price;
	std::array<PriceLevel, MaxLevels> levels{};

public:
	explicit BookSide(Price base)
		: base_price(base), best_price(INVALID_PRICE) {}

	inline bool add(Price price, Qty qty, bool is_bid) noexcept
	{
		if (!is_valid_price(price))
			return false;

		const size_t idx = index_of(price);
		levels[idx].volume += qty;

		if (best_price == INVALID_PRICE || (is_bid && price > best_price))
		{
			best_price = price;
		}
		else if (!is_bid && price < best_price)
		{
			best_price = price;
		}

		return true;
	}

	inline bool cancel(Price price, Qty qty, bool is_bid) noexcept
	{
		if (!is_valid_price(price))
			return false;

		const size_t idx = index_of(price);
		auto &level = levels[idx];

		if (level.volume < qty)
			return false;
		level.volume -= qty;

		if (level.volume == 0 && price == best_price)
		{
			best_price = find_next_best(is_bid);
		}

		return true;
	}

	inline void trade(Price price, Qty qty) noexcept
	{
		cancel(price, qty, true); // TODO: This is incorrect. This is dummy trade
	}

	inline Price best() const noexcept { return best_price; }

	inline Qty volume_at(Price price) const noexcept
	{
		return levels[index_of(price)].volume;
	}

private:
	inline bool is_valid_price(Price price) const noexcept
	{
		return price >= base_price &&
			   price < base_price + static_cast<Price>(MaxLevels);
	}

	inline size_t index_of(Price price) const noexcept
	{
		return static_cast<size_t>(price - base_price);
	}

	Price find_next_best(bool is_bid) noexcept
	{
		switch (is_bid)
		{
		case true:
			for (int64_t p = static_cast<int64_t>(best_price) - 1;
				 p >= static_cast<int64_t>(base_price);
				 --p)
			{
				if (levels[index_of(static_cast<Price>(p))].volume > 0)
				{
					return static_cast<Price>(p);
				}
			}
			break;

		case false:
			for (int64_t p = static_cast<int64_t>(base_price);
				 p < static_cast<int64_t>(best_price);
				 ++p)
			{
				if (levels[index_of(static_cast<Price>(p))].volume > 0)
				{
					return static_cast<Price>(p);
				}
			}
			break;

		default:
			break;
		}
		return INVALID_PRICE;
	}
};
