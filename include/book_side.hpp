#pragma once
#include <array>
#include "types.hpp"

struct PriceLevel {
    Qty volume = 0;
};

template <size_t MaxLevels>
class BookSide {
private:
        Price base_price;
        Price best_price;
        std::array<PriceLevel, MarketEvent> levels{};

public:
        explicit BookSide(Price base)
                : base_price(base), best_price(INVALID_PRICE) {}

        inline void add(Price price, Qty qty) noexcept {
		const size_t idx = index_of(price);
		levels[idx].volume += qty;

		if(best_price == INVALID_PRICE || price > best_price) {
			best_price = price;
		}
	}

        inline void cancel(Price price, Qty qty) noexcept {
		const size_t idx = index_of(price);
		auto& level = levels[idx];

		level.volume -= qty;

		if(level.volume == 0 && price == best_price) {
			best_price = find_next_best();
		}
	}

        inline void trade(Price price, Qty qty) noexcept {
		cancel(price, qty);
	}

        inline Price best() const noexcept { return best_price; }

        inline Qty volume_at(Price price) const noexcept {
		return levels[index_of(price)].volume;		
	}

private:
        inline size_t index_of(Price price) const noexcept {
                return static_cast<size_t>(price - base_price);
        }

        Price find_next_best() noexcept {
		for(Price p = best_price-1; p >= base_price; --p) {
			if(levels[index_of(p)].volume > 0) {
				return p;
			}
		}
		return INVALID_PRICE;
	}
};
