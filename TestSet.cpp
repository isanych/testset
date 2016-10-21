#include <random>
#include <unordered_set>
#include <iostream>
#include <set>
#include <iomanip>
#include "btree_set.h"
#include <sparsehash/sparse_hash_set>
#include <sparsehash/dense_hash_set>
#include <deque>
#include <mct/hash-set.hpp>

#ifdef _DEBUG
const auto amount = 100000;
#else
const auto amount = 10000000;
#endif
const auto mem_offset = sizeof(size_t);

int64_t mem_counter = 0;
int64_t new_counter = 0;
int64_t del_counter = 0;

int64_t reset()
{
	auto ret = mem_counter;
	mem_counter = 0;
	new_counter = 0;
	del_counter = 0;
	return ret;
}

#ifdef WRAP_ALLOC
void* operator new(size_t sz) {
	mem_counter += sz;
	++new_counter;
	auto ret = static_cast<size_t*>(malloc(sz + mem_offset));
	*ret = sz;
	return ret + 1;
}

void operator delete(void* ptr) noexcept
{
	auto sptr = static_cast<size_t *>(ptr) - 1;
	mem_counter -= *sptr;
	++del_counter;
	free(sptr);
}
#endif

size_t populate_count = amount;
size_t multiplier = 0;

template<typename T>
void init_set(T& s){}

template<>
void init_set(google::dense_hash_set<int64_t>& s)
{
	s.set_empty_key(0);
}

struct Pool
{
	const size_t pool_size = 16300;
	Pool() noexcept : _next(), _end() {}
	char* grow(size_t n) {
		_pools.emplace_back(static_cast<char*>(operator new(n)));
		return _pools.back().get();
	}
	void* allocate(size_t n) {
		if (n == 0)
		{
			return nullptr;
		}
		if (static_cast<size_t>(_end - _next) < n) {
			if (pool_size <= n) {
				return grow(n);
			}
			_next = grow(pool_size);
			_end = _next + pool_size;
		}
		auto ret = _next;
		_next += n;
		return ret;
	}
	void clear()
	{
		_next = _end = nullptr;
		_pools.clear();
	}
private:
	std::deque<std::unique_ptr<char>> _pools;
	char* _next;
	char* _end;
};

template<typename T>
struct PoolAllocator
{
	using value_type = T;
	using size_type = size_t;
	using pointer = value_type*;
	using reference = value_type&;
	using const_pointer = const value_type*;
	using const_reference = const value_type&;
	using difference_type = typename std::pointer_traits<pointer>::difference_type;
	size_type max_size() const { return std::numeric_limits<size_type>::max(); }

    PoolAllocator() noexcept {}
    PoolAllocator(const PoolAllocator<value_type>&) noexcept {}
	template<typename U>
	PoolAllocator(const PoolAllocator<U>&) noexcept {}

	pointer allocate(size_t size)
	{
		return static_cast<pointer>(_pool.allocate(size * sizeof(value_type)));
	}

	void deallocate(pointer , size_t) {}

	template<typename U>
	struct rebind
	{
		typedef PoolAllocator<U> other;
	};

private:
	static Pool _pool;
};

template<typename T>
Pool PoolAllocator<T>::_pool;

template<typename T, typename U>
constexpr bool operator==(const PoolAllocator<T> &, const PoolAllocator<U> &) noexcept { return true; }

template<typename T, typename U>
constexpr bool operator!=(const PoolAllocator<T> &,	const PoolAllocator<U> &) noexcept { return false; }

template<typename T>
void test(const char* name)
{
	std::uniform_int_distribution<int64_t> rnd(std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max());
	T s;
	init_set(s);
	{
		std::default_random_engine generator(5489);
		reset();
		for (auto n = 0; n < populate_count; ++n)
		{
			s.insert(rnd(generator));
		}
	}
	auto nc = new_counter;
	auto dc = del_counter;
	auto memory_size = reset();
	auto data_size = s.size() * sizeof(int64_t);
	std::cout << name << ": " << data_size / 1024 << " KB of data " << memory_size / 1024 << " KB allocated, ratio " << std::setprecision(2) << static_cast<double>(memory_size) / data_size << " new cnt: " << nc << " del cnt: "<< dc <<std::endl;
	size_t cnt = 0;
	{
		std::default_random_engine generator(5489);
		for (auto n = 0; n < populate_count * multiplier; ++n)
		{
			cnt += s.find(rnd(generator)) != s.end() ? 1 : 0;
		}
	}
	std::cout << cnt << std::endl;
}

int main(int argc, char** argv)
{
	try
	{
		if (argc == 1)
		{
			std::cout << "Usage: test [<cnt>...] <type> " << std::endl;
			std::cout << " s   - std::set" << std::endl;
			std::cout << " u   - std::unordered_set" << std::endl;
			std::cout << " up  - std::unordered_set with pool" << std::endl;
			std::cout << " b   - btree::btree_set" << std::endl;
			std::cout << " sp  - google::sparse_hash_set" << std::endl;
			std::cout << " spp - google::sparse_hash_set with pool" << std::endl;
			std::cout << " d   - google::dense_hash_set" << std::endl;
			std::cout << " dp  - google::dense_hash_set with pool" << std::endl;
			std::cout << " c   - mct::closed_hash_set" << std::endl;
			std::cout << " f   - mct::forward_hash_set" << std::endl;
			std::cout << " hf  - mct::huge_forward_hash_set" << std::endl;
			std::cout << " hl  - mct::huge_linked_hash_set" << std::endl;
			return 0;
		}
		for (int i = 1; i < argc; ++i)
		{
			std::string s(argv[i]);
			if (s == "s")
			{
				test<std::set<int64_t>>("set");
			}
			else if (s == "u")
			{
				test<std::unordered_set<int64_t>>("unordered_set");
			}
			else if (s == "up")
			{
				test<std::unordered_set<int64_t, std::hash<int64_t>, std::equal_to<int64_t>, PoolAllocator<int64_t>>>("unordered_set_pool");
			}
			else if (s == "b")
			{
				test<btree::btree_set<int64_t>>("btree_set");
			}
			else if (s == "sp")
			{
				test<google::sparse_hash_set<int64_t>>("sparse");
			}
			else if (s == "spp")
			{
				test<google::sparse_hash_set<int64_t, std::hash<int64_t>, std::equal_to<int64_t>, PoolAllocator<int64_t>>>("sparse_pool");
			}
			else if (s == "d")
			{
				test<google::dense_hash_set<int64_t>>("dense");
			}
			else if (s == "dp")
			{
				test<google::dense_hash_set<int64_t, std::hash<int64_t>, std::equal_to<int64_t>, PoolAllocator<int64_t>>>("dense_pool");
			}
			else if (s == "c")
			{
				test<mct::closed_hash_set<int64_t>>("closed");
			}
			else if (s == "f")
			{
				test<mct::forward_hash_set<int64_t>>("forward");
			}
			else if (s == "hf")
			{
				test<mct::huge_forward_hash_set<int64_t>>("huge_forward");
			}
			else if (s == "hl")
			{
				test<mct::huge_linked_hash_set<int64_t>>("huge_linked");
			}
			else
			{
				auto n = std::stoull(s);
				if (n < 1000)
				{
					multiplier = n;
				}
				else
				{
					populate_count = n;
				}
			}
		}
	}
	catch(const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
	return 0;
}
