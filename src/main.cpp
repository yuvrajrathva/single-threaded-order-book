#include "order_book.hpp"
#include "market_event.hpp"
#include "publisher.hpp"
#include "spsc.hpp"
#include "mpmc.hpp"

#include <thread>
#include <iostream>
#include <atomic>
#include <immintrin.h>

int main() {
	constexpr size_t QSIZE = 1 << 14;

	mpmc<MarketEvent> event_q(QSIZE);
	spsc<TopOfBook> md_q(QSIZE);

	OrderBook book(1000, 1000);
	MarketDataPublisher<spsc<TopOfBook>> publisher(md_q);
	
	std::atomic<bool> producers_done{false};	
	// Start producer threads
	std::thread producer([&]() {
		MarketEvent ev{};
		for (Price p=1000; p<1005; ++p) {
			ev = {EventType::Add, p, 10, true};
			while(!event_q.push(ev)) _mm_pause();
		
			ev = {EventType::Add, p+10, 10, false};
			while(!event_q.push(ev)) _mm_pause();
		}
	       
                // 2️⃣ Cancel best bid
	        ev = {EventType::Cancel, 1004, 10, true};
	        while (!event_q.push(ev)) _mm_pause();

	        // 3️⃣ Cancel best ask
	        ev = {EventType::Cancel, 1010, 10, false};
	        while (!event_q.push(ev)) _mm_pause();

	        // 4️⃣ Aggressive BUY → Trade against best ask
	        ev = {EventType::Trade, 1011, 5, true};
	        while (!event_q.push(ev)) _mm_pause();

	        // 5️⃣ Aggressive SELL → Trade against best bid
	        ev = {EventType::Trade, 1003, 7, false};
	        while (!event_q.push(ev)) _mm_pause();

	        // 6️⃣ Refill liquidity
	        ev = {EventType::Add, 1006, 20, true};
	        while (!event_q.push(ev)) _mm_pause();

	        ev = {EventType::Add, 1012, 20, false};
	        while (!event_q.push(ev)) _mm_pause();

		producers_done.store(true, std::memory_order_release);
	});

	// Start order book thread
	std::thread ob_thread([&]() {
		MarketEvent ev{};
		while(true) {
			if(event_q.pop(ev)) {
				book.on_event(ev);
				publisher.publish(book);
			} else {
				if(producers_done.load(std::memory_order_acquire)) {
					break;
				}
				_mm_pause();
			}
		}
	});

	std::thread consumer([&]() {
		TopOfBook tob{};
		while(true) {
			if(md_q.pop(tob)) {
				std::cout
					<<"BID="<<tob.best_bid
					<<" ASK="<<tob.best_ask
					<<" SPR="<<tob.spread<<"\n";
			} else {
				if(producers_done.load(std::memory_order_acquire) &&
					md_q.empty()) {
					break;
				}
				_mm_pause();
			}
		}
	});

	// Join threads	
	producer.join();
	ob_thread.join();
	consumer.join();
	return 0;
}
