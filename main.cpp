

template <size_t MaxLevels>
inline void BookSide<MaxLevels>::add(Price price, Qty qty) noexcept {
	const size_t idx = index_of(price);
	levels[idx].volume += qty;

	if(best_price == INVALID_PRICE || price > best_price) {
		best_price = price;
	}
}

template <size_t MaxLevels>
inline void BookSide<MaxLevels>::cancel(Price price, Qty qty) noexcept {
	const size_t idx = index_of(price);
	auto& level = levels[idx];

	level.volume -= qty;
		
	if(level.volume == 0 && price == best_price) {
		best_price = find_next_best();
	}
}

template <size_t MaxLevels>
inline void BookSide<MaxLevels>::trade(Price price, Qty qty) noexcept {
	cancel(price, qty);	
}

template <size_t MaxLevels>
Price BookSize<MaxLevels>::find_next_best() noexcept {
	for(Price p = best_price-1; p >= base_price; --p) {
		if(levels[index_of(p)].volume > 0) {
			return p;
		}
	}
	return INVALID_PRICE;
}

inline void OrderBook::on_event(const MarketEvent& ev) noexcept {
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
