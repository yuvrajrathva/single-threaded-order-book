#pragma once
#include <array>
#include "types.hpp"

struct PriceLevel
{
	Qty volume = 0;
};

uint32_t TICK_SIZE = 1; // For now I kept it 1 for simplicity

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
		if (price < base_price)
			return false;

		const Price delta = price - base_price;
		const Price level = delta / TICK_SIZE;

		return (delta == (level * TICK_SIZE)) && (level < MaxLevels);
	}

	inline size_t index_of(Price price) const noexcept
	{
		const Price delta = price - base_price;
		return static_cast<size_t>(delta / TICK_SIZE);
	}

	Price find_next_best(bool is_bid) noexcept
	{
		size_t start = index_of(best_price);

		switch (is_bid)
		{
		case true:
			for (size_t i = start; i-- > 0; )
			{
				if (levels[i].volume > 0)
				{
					return base_price + (i * TICK_SIZE);
				}
			}
			break;

		case false:
			for (size_t i = start+1;
				 i < MaxLevels;
				 ++i)
			{
				if (levels[i].volume > 0)
				{
					return base_price + (i * TICK_SIZE);
				}
			}
			break;

		default:
			break;
		}
		return INVALID_PRICE;
	}
};
