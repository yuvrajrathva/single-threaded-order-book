#pragma once // compiler will only process its content once

#include <atomic>
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>
#include <stdexcept>
#include <memory>

template<typename T, typename Alloc = std::allocator<T>>
class mpmc
{
private:
	struct alignas(64) CellCtrl {
	    std::atomic<size_t> seq;
	};

	struct CellData {
	    alignas(alignof(T)) unsigned char storage_bytes[sizeof(T)];
	};

        static T* storage_ptr(CellData& d) noexcept {
        	return std::launder(reinterpret_cast<T*>(d.storage_bytes));
        }
	

	Alloc alloc_;
	CellCtrl* ctrl_;
	CellData* data_;
	const size_t size_;
	const size_t mask_;
	
	alignas(64) std::atomic<size_t> head_{0}; // read
	char pad2[64 - sizeof(std::atomic<size_t>)];

	alignas(64) std::atomic<size_t> tail_{0}; // write
	char pad3[64 - sizeof(std::atomic<size_t>)];

public:
	explicit mpmc(size_t size_pow2, const Alloc& alloc = Alloc())
	: alloc_(alloc),
	  ctrl_(nullptr),
	  data_(nullptr),
	  size_(size_pow2),
	  mask_(size_pow2-1)
	{
		if(((size_pow2 & (size_pow2-1)) != 0) || size_pow2 == 0) {
			throw std::invalid_argument("size must be a power of two");
		}

		ctrl_ = static_cast<CellCtrl*>(::operator new[](sizeof(CellCtrl) * size_));
		data_ = static_cast<CellData*>(::operator new[](sizeof(CellData) * size_));
		for(size_t i=0; i<size_; i++){
			new (&ctrl_[i].seq) std::atomic<size_t>(i);
		}		
	}

	~mpmc(){
		// slot should contain a contructed T
		if constexpr (!std::is_trivially_destructible_v<T>) {
			T temp;
			while(pop(temp)) {}
		}

		::operator delete[](ctrl_);
		::operator delete[](data_);
	}

	// Non-copyable, non-movable
	mpmc(const mpmc&) = delete;
	mpmc& operator=(const mpmc&) = delete;

	bool push(const T& val) noexcept {
		return emplace(val);
	}

	bool pop(T& out) noexcept {
		size_t pos = head_.load(std::memory_order_relaxed);
		while(true) {
			size_t idx = pos & mask_;
			CellCtrl& c = ctrl_[idx];
			size_t seq = c.seq.load(std::memory_order_acquire);
			ptrdiff_t diff = static_cast<ptrdiff_t>(seq) - static_cast<ptrdiff_t>(pos+1);
	
			if(__builtin_expect(diff == 0,1)) {
				// attempt to claim this slot
				if(head_.compare_exchange_weak(pos, pos+1, std::memory_order_relaxed, std::memory_order_relaxed)) {
					// now we own this slot
					T* src = storage_ptr(data_[idx]);
					out = std::move(*src);
					src->~T();

					c.seq.store(pos + size_, std::memory_order_release); // marks it empty
					return true;
				}
				continue;
				// CAS failed -> pos updated, retry immediately
			}
			else if(diff < 0) {
				// seq < head + 1 => slot is not yet produced
				return false;
			}
			else {
				// slot not yet for us (other consumer moved head forward)
				pos = head_.load(std::memory_order_relaxed);
			}
		}
	}

	size_t capacity() const noexcept { return size_; };

private:
	// internal emplace to avoid code duplication
	template<typename... Args>
	bool emplace(Args&&... args) noexcept {
		size_t pos = tail_.load(std::memory_order_relaxed);
		while(true) {
			size_t idx = pos & mask_;
			CellCtrl& c = ctrl_[idx];
			size_t seq = c.seq.load(std::memory_order_acquire);
			ptrdiff_t diff = static_cast<ptrdiff_t>(seq) - static_cast<ptrdiff_t>(pos);

			if(__builtin_expect(diff == 0,1)) {
				size_t old_pos = pos;
				if(tail_.compare_exchange_weak(pos, pos+1, std::memory_order_relaxed, std::memory_order_relaxed)) {
					T* dest = storage_ptr(data_[idx]);
					new (dest) T(std::forward<Args>(args)...);

					c.seq.store(pos+1, std::memory_order_release);
					return true;
				}
				continue;		
			}
			else if(diff < 0) {
				return false;
			}
			else {
				pos = tail_.load(std::memory_order_relaxed);
			}
		}
	}
	
};	
