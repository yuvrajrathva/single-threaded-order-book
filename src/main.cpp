#include "order_book.hpp"
#include "market_event.hpp"
#include "publisher.hpp"
#include "lat_helper.hpp"
#include "spsc.hpp"
#include "mpmc.hpp"

#include <thread>
#include <iostream>
#include <atomic>
#include <vector>
#include <immintrin.h>

int main()
{
	constexpr size_t QSIZE = 1 << 14;

	mpmc<MarketEvent> event_q(QSIZE);
	spsc<TopOfBook> md_q(QSIZE);

	OrderBook book(1000, 1000);
	MarketDataPublisher<spsc<TopOfBook>> publisher(md_q);

	std::atomic<bool> producers_done{false};
	// Start producer threads
	std::thread producer([&]()
						 {
		MarketEvent ev{};
		for (Price p=1000; p<1005; ++p) {
			ev = {EventType::Add, p, 10, true};
			while(!event_q.push(ev)) _mm_pause();
		
			ev = {EventType::Add, p+10, 10, false};
			while(!event_q.push(ev)) _mm_pause();
		}
	       
        	// Cancel best bid
	        ev = {EventType::Cancel, 1004, 10, true};
	        while (!event_q.push(ev)) _mm_pause();

	        // Cancel best ask
	        ev = {EventType::Cancel, 1010, 10, false};
	        while (!event_q.push(ev)) _mm_pause();

	        //  Refill liquidity
	        ev = {EventType::Add, 1006, 20, true};
	        while (!event_q.push(ev)) _mm_pause();

	        ev = {EventType::Add, 1012, 20, false};
	        while (!event_q.push(ev)) _mm_pause();

		producers_done.store(true, std::memory_order_release); });

	std::vector<uint64_t> latencies;
	latencies.reserve(1'000'000);

	// Start order book thread
	std::thread ob_thread([&]()
						  {
		MarketEvent ev{};

		while(true) {
			if(event_q.pop(ev)) {
				uint64_t t0 = rdtsc_now();
				
				book.on_event(ev);
				publisher.publish(book);

				uint64_t t1 = rdtsc_now();
				latencies.push_back(t1 - t0);
			} else {
				if(producers_done.load(std::memory_order_acquire)) {
					break;
				}
				_mm_pause();
			}
		} });

	std::thread consumer([&]()
						 {
		TopOfBook tob{};
		while(true) {
			if(md_q.pop(tob)) {
				std::cout<<"Best bid: "<<tob.best_bid
						<<" Best ask: "<<tob.best_ask
						<<" Spread: "<<tob.spread<<"\n";
			} else {
				if(producers_done.load(std::memory_order_acquire) &&
					md_q.empty()) {
					break;
				}
				_mm_pause();
			}
		} });

	// Join threads
	producer.join();
	ob_thread.join();
	consumer.join();

	std::sort(latencies.begin(), latencies.end());
	auto pct = [&](double p)
	{
		size_t idx = static_cast<size_t>(p * latencies.size());
		return latencies[std::min(idx, latencies.size() - 1)];
	};

	double ghz = calibrate_ghz();

	auto to_ns = [&](uint64_t cyc)
	{
		return cycles_to_ns(cyc, ghz);
	};

	std::cout << "Engine latency (OrderBook + ToB publish)\n";
	std::cout << "P50  : " << to_ns(pct(0.50)) << " ns\n";
	std::cout << "P90  : " << to_ns(pct(0.90)) << " ns\n";
	std::cout << "P99  : " << to_ns(pct(0.99)) << " ns\n";
	std::cout << "P999 : " << to_ns(pct(0.999)) << " ns\n";
	std::cout << "Min  : " << to_ns(latencies.front()) << " ns\n";
	std::cout << "Max  : " << to_ns(latencies.back()) << " ns\n";

	return 0;
}
