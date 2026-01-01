

using Price = uint32_t;
using Qty = uint32_t;

constexpr size_t MAX_PRICE_LEVELS = 4096;
constexpr Price INVALID_PRICE = -1;

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

struct PriceLevel {
	Qty volume = 0;
};

template <size_t MaxLevels>
class BookSide {
private:
	Price base_price;
	Price best_price;
	std::array<PriceLevel, MarketEvent> levels;

public:
	explicit BookSide(Price base)
		: base_price(base), best_price(base) {}

	inline void add(Price price, Qty qty) noexcept;
	inline void cancel(Price price, Qty qty) noexcept;
	inline void trade(Price price, Qty qty) noexcept;

	inline Price best() const noexcept { return best_price; }
	inline Qty volume_at(Price price) const noexcept;

private:
	inline size_t index_of(Price price) const noexcept {
		return static_cast<size_t>(price - base_price);
	}

	Price find_next_best() noexcept;
};


class OrderBook {
public:
	OrderBook(Price bid_base, Price ask_base)
		: bids(bid_base), asks(ask_base) {}
	
	inline void on_event(const MarketEvent& ev) noexcept;

	inline Price best_bid() const noexcept { return bids.best(); }
	inline Price best_ask() const noexcept { return asks.best(); }

private:
	BookSide<MAX_PRICE_LEVELS> bids;
	BookSide<MAX_PRICE_LEVELS> asks;	
};


struct TopOfBook {
	Price best_bid;
	Price best_ask;
	Price spread;
}

inline Price spread(Price bid, Price ask) noexcept {
	if(bid == INVALID_PRICE || ask == INVALID_PRICE) return INVALID_PRICE;
	return ask - bid;
}


















