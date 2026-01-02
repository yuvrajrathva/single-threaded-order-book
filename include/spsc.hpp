#include <atomic>
#include <stdexcept>
#include <memory>
#include <type_traits>
#include <cstddef>
#include <cstdint>

template <typename T, typename Alloc = std::allocator<T>>
class spsc {
private:
	using allocator_traits = std::allocator_traits<Alloc>;
	Alloc alloc_;
	T* buffer_;
	const std::size_t size_;
	const std::size_t mask_;

	alignas(64) std::atomic<uint64_t> head_{0}; // read
	char pad1[64 - sizeof(std::atomic<uint64_t>)];	

	alignas(64) std::atomic<uint64_t> tail_{0}; // write
	char pad2[64 - sizeof(std::atomic<uint64_t>)];	

public:
	explicit spsc(size_t size_pow2, const Alloc& alloc = Alloc())
	: alloc_(alloc),
	  size_(size_pow2),
	  mask_(size_pow2-1)
	{
		if((size_pow2 & (size_pow2-1)) != 0) {
			throw std::invalid_argument("size must be a power of two");
		}

		buffer_ = allocator_traits::allocate(alloc_, size_);
		for(std::size_t i=0; i<size_; i++) {
			allocator_traits::construct(alloc_, buffer_+i);
		}
	}

	~spsc() {
		destroy_remaining();
		allocator_traits::deallocate(alloc_, buffer_, size_);
	}

	// Non-copyable, non-movable
	spsc(const spsc&) = delete;
	spsc& operator=(const spsc&) = delete;
	
	bool push(const T& value) noexcept {
		return	emplace(value);
	}

	bool pop(T& out) noexcept {
		const auto head = head_.load(std::memory_order_relaxed);
		const auto tail = tail_.load(std::memory_order_acquire);

		if(head == tail) {
			return false;
		}

		out = buffer_[head & mask_];
		
		head_.store(head+1, std::memory_order_release);
		return true;
	}

	bool empty() const noexcept {
		return head_.load(std::memory_order_acquire) ==
			tail_.load(std::memory_order_acquire);
	}

	bool full() const noexcept {
		const auto tail = tail_.load(std::memory_order_relaxed);
		const auto head = head_.load(std::memory_order_acquire);

		return ((tail+1) & mask_) == (head & mask_);
	}

	std::size_t capacity() const noexcept { return size_; }

private:
	template<typename... Args>
	bool emplace(Args&&... args) noexcept {
		const auto tail = tail_.load(std::memory_order_relaxed);
		const auto head = head_.load(std::memory_order_acquire);

		if(((tail+1) & mask_) == (head & mask_)) {
			return false;
		}

		new (&buffer_[tail & mask_]) T(std::forward<Args>(args)...); 

		tail_.store(tail+1, std::memory_order_release);
		return true;
	}

	void destroy_remaining() {
		if constexpr(!std::is_trivially_destructible_v<T>) {
			const auto head = head_.load(std::memory_order_relaxed);
			const auto tail = tail_.load(std::memory_order_relaxed);
			for(auto i=head; i!=tail; i++) {
				allocator_traits::destroy(alloc_, buffer_+(i & mask_));
			}
		}	
	}	
};
