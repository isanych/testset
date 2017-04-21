#ifdef _MSC_VER
#include <stdint.h>
#include <intrin.h>

int32_t inline __builtin_ctz(uint32_t value)
{
  unsigned long trailing_zero = 0;

  if (_BitScanForward(&trailing_zero, value))
  {
    return trailing_zero;
  }
  // This is undefined, better choose 32 than 0
  return 32;
}

#if defined(_M_X64) && !defined(OLD_CPU)

int32_t inline __builtin_clz(uint32_t value)
{
  return __lzcnt(value);
}

#else

int32_t inline __builtin_clz(uint32_t value)
{
  unsigned long leading_zero = 0;

  if (_BitScanReverse(&leading_zero, value))
  {
    return 31 - leading_zero;
  }
  // This is undefined, better choose 32 than 0
  return 32;
}

#endif

int32_t inline __builtin_popcount(uint32_t value)
{
  return __popcnt(value);
}

#ifdef _M_X64

int32_t inline __builtin_ctzll(uint64_t value)
{
  unsigned long trailing_zero = 0;

  if (_BitScanForward64(&trailing_zero, value))
  {
    return trailing_zero;
  }
  return 64;
}

#ifndef OLD_CPU

int32_t inline __builtin_clzll(uint64_t value)
{
  return static_cast<int32_t>(__lzcnt64(value));
}

#else

int32_t inline __builtin_clzll(uint64_t value)
{
  unsigned long leading_zero = 0;

  if (_BitScanReverse64(&leading_zero, value))
  {
    return 63 - leading_zero;
  }
  return 64;
}

#endif

int32_t inline __builtin_popcountll(uint64_t value)
{
  return static_cast<int32_t>(__popcnt64(value));
}

#endif
#endif

#include <random>
#include <unordered_set>
#include <iostream>
#include <set>
#include <iomanip>
#include <chrono>
#include "btree_set.h"
#include <sparsehash/sparse_hash_set>
#include <sparsehash/dense_hash_set>
#include <deque>
#include <mct/hash-set.hpp>
#include <sdsl/bit_vectors.hpp>
#include "ewah.h"
#include "sparse_sets.h"
#include "concise.h"

#ifdef _DEBUG
const auto amount = 100000;
#else
const auto amount = 10000000;
#endif
const auto mem_offset = sizeof(size_t);

#ifdef TEST32
typedef uint32_t test_t;
#else
typedef uint64_t test_t;
#endif
test_t max_value = std::numeric_limits<test_t>::max();

struct Counter
{
  int64_t bytes = 0;
  int64_t times = 0;
  void add(int64_t bytes)
  {
    ++times;
    this->bytes += bytes;
  }
};

struct Pool
{
  size_t pool_chunk_size = 16300;
  bool is_pool = false;

  char* grow(size_t sz) {
    _new.add(sz);
    _pools.emplace_back(static_cast<char*>(_allocate(sz)));
    return _pools.back().get();
  }
  void clear()
  {
    _next = _end = nullptr;
    _pools.clear();
  }
  void* allocate(size_t sz) {
    if (sz == 0)
    {
      return nullptr;
    }
#ifdef USE_POOL
    if (static_cast<size_t>(_end - _next) < sz) {
      if (pool_chunk_size <= sz) {
        return grow(sz);
      }
      _next = grow(pool_chunk_size);
      _end = _next + pool_chunk_size;
    }
    auto ret = _next;
    _next += sz;
    return ret;
#else
    return _allocate(sz);
#endif
  }
  void deallocate(void* p, size_t sz) {
    _free.add(sz);
#ifndef USE_POOL
    free(p);
#endif
  }
  void* reallocate(void* p, size_t new_sz, size_t old_sz) {
    if (new_sz == old_sz)
    {
      return p;
    }
    if (new_sz > old_sz)
    {
      _grow.add(new_sz - old_sz);
#ifdef USE_POOL
      auto new_p = allocate(new_sz);
      memmove(new_p, p, std::min(old_sz, new_sz));
      deallocate(p, old_sz);
      return new_p;
#endif
    }
    else
    {
      _shrink.add(old_sz - new_sz);
#ifdef USE_POOL
      return p;
#endif
    }
#ifndef USE_POOL
    return realloc(p, new_sz);
#endif
  }

  static void report(const char* name, size_t count, size_t sz)
  {
    if (count == 0 && sz == 0)
    {
      return;
    }
    std::cout << name << ": ";
    if (count != 0)
    {
      std::cout << count << " times ";
    }
    std::cout << sz << " bytes ";
    std::cout << sz / (1024 * 1024) << " MB";
    if (count != 0)
    {
      std::cout << " average " << sz / count << " bytes";
    }
    std::cout << std::endl;
  }

  static void report(const char* name, Counter c) { report(name, c.times, c.bytes); }

  void report(size_t data_size)
  {
    auto used = _alloc.bytes + _grow.bytes - _shrink.bytes  - _free.bytes;
    report("Data size", 0, data_size);
    report("Used", 0, used);
    report("Alloc", _alloc);
    report("Free", _free);
    report("Grow", _grow);
    report("Shrink", _shrink);
    report("New", _new);
    report("Delete", _delete);
    if (used > 0)
    {
      std::cout << "Ratio: " << std::fixed << std::setprecision(3) << static_cast<double>(used) / data_size << std::endl;
    }
  }
private:
  void* _allocate(size_t sz) {
    _alloc.add(sz);
    return malloc(sz);
  }
  void _deallocate(void* p, size_t sz) {
    _free.add(sz);
    free(p);
  }

  std::deque<std::unique_ptr<char>> _pools;
  char* _next = nullptr;
  char* _end = nullptr;
  Counter _alloc;
  Counter _free;
  Counter _grow;
  Counter _shrink;
  Counter _new;
  Counter _delete;
};

Pool pool;

#ifdef WRAP_ALLOC
template<class T>
class Reallocator : google::libc_allocator_with_realloc<T> {
public:
  using value_type = T;
  using size_type = size_t;
  using pointer = value_type*;
  using reference = value_type&;
  using const_pointer = const value_type*;
  using const_reference = const value_type&;
  using difference_type = typename std::pointer_traits<pointer>::difference_type;
  size_type max_size() const { return std::numeric_limits<size_type>::max(); }

  Reallocator() noexcept {}
  Reallocator(const Reallocator<value_type>&) noexcept {}
  template<typename U>
  Reallocator(const Reallocator<U>&) noexcept {}

  pointer allocate(size_type n) {
    return static_cast<pointer>(pool.allocate(n * sizeof(value_type)));
  }
  void deallocate(pointer p, size_type n) {
    pool.deallocate(p, n);
  }
  pointer reallocate(pointer p, size_type new_n, size_type old_n) {
    return static_cast<pointer>(pool.reallocate(p, new_n * sizeof(value_type), old_n * sizeof(value_type)));
  }
  pointer address(reference r) const { return &r; }
  const_pointer address(const_reference r) const { return &r; }
  void construct(pointer p, const value_type& val) {
    new(p) value_type(val);
  }
  void destroy(pointer p) { p->~value_type(); }

  template<class U>
  struct rebind {
    typedef Reallocator<U> other;
  };
};

template<>
class Reallocator<void> {
public:
  typedef void value_type;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;
  typedef void* pointer;
  typedef const void* const_pointer;

  template<class U>
  struct rebind {
    typedef Reallocator<U> other;
  };
};

template<class T>
constexpr bool operator==(const Reallocator<T>&, const Reallocator<T>&) noexcept { return true; }

template<class T>
constexpr bool operator!=(const Reallocator<T>&, const Reallocator<T>&) noexcept { return false; }

template <class T>
using PoolAllocator = Reallocator<T>;

#else
template <class T>
using Reallocator = google::libc_allocator_with_realloc<T>;
template <class T>
using PoolAllocator = std::allocator<T>;
#endif

typedef google::dense_hash_set<test_t, std::hash<test_t>, std::equal_to<test_t>, Reallocator<test_t>> Dense;

size_t populate_count = amount;
size_t pop_hit_count = 0;
size_t hit_count = 0;
size_t miss_count = 0;

template<typename T>
void init_set(T& s){}

template<>
void init_set(Dense& s)
{
  s.set_empty_key(0);
}

template<>
void init_set(sdsl::bit_vector& s)
{
  s.resize(populate_count);
}

template<typename T>
void elapsed(const char* name, T end, T start)
{
  std::cout << name << ", sec: " << std::fixed << std::setprecision(2) <<
    std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.0 << std::endl;
}

template<typename T>
void do_set(T& s, test_t v)
{
  s.insert(v);
}

template<>
void do_set(sdsl::bit_vector& s, test_t v)
{
  s[v % populate_count] = true;
}

template<>
void do_set(EWAHBoolArray<uint32_t>& s, test_t v)
{
  s.set(static_cast<uint32_t>(v));
}

template<>
void do_set(EWAHBoolArray<uint64_t>& s, test_t v)
{
  s.set(v);
}

template<>
void do_set(ConciseSet<>& s, test_t v)
{
  s.add(static_cast<uint32_t>(v));
}

template<typename T>
bool is_set(T& s, test_t v)
{
  return s.find(v) != s.end();
}

template<>
bool is_set(sdsl::bit_vector& s, test_t v)
{
  return s[v % populate_count];
}

template<>
bool is_set(EWAHBoolArray<uint32_t>& s, test_t v)
{
  return s.get(static_cast<uint32_t>(v));
}

template<>
bool is_set(EWAHBoolArray<uint64_t>& s, test_t v)
{
  return s.get(static_cast<uint64_t>(v));
}

template<>
bool is_set(sparse_set& s, test_t v)
{
  return s.test(v);
}

template<>
bool is_set(unordered_sparse_set& s, test_t v)
{
  return s.test(v);
}

template<>
bool is_set(ConciseSet<>& s, test_t v)
{
  return s.contains(static_cast<uint32_t>(v));
}

std::string test_name;

template<typename T>
void test()
{
  std::cout << "Testing: " << test_name << " max " << max_value << " bits " << 64 - __builtin_clzll(max_value) << " cnt " << populate_count;
#ifdef WRAP_ALLOC
  std::cout << " wrap_alloc";
#endif
#ifdef USE_POOL
  std::cout << " pool";
#endif
  std::cout << std::endl;
  std::uniform_int_distribution<test_t> rnd(0, max_value);
  T s;
  init_set(s);
  std::default_random_engine generator(5489);
  {
    auto start = std::chrono::high_resolution_clock::now();
    for (auto n = 0; n < populate_count; ++n)
    {
      do_set(s, rnd(generator));
    }
    auto end = std::chrono::high_resolution_clock::now();
    pool.report(populate_count * sizeof(test_t));
    elapsed("population", end, start);
  }
  size_t cnt = 0;
  {
    auto start = std::chrono::high_resolution_clock::now();
    for (auto h = 0; h < pop_hit_count; ++h)
    {
      std::default_random_engine gen(5489);
      for (auto n = 0; n < populate_count; ++n)
      {
        do_set(s, rnd(gen));
      }
    }
    auto end = std::chrono::high_resolution_clock::now();
    elapsed("population hit", end, start);
  }
  {
    auto start = std::chrono::high_resolution_clock::now();
    for (auto h = 0; h < hit_count; ++h)
    {
      std::default_random_engine gen(5489);
      for (auto n = 0; n < populate_count; ++n)
      {
        cnt += is_set(s, rnd(gen)) ? 1 : 0;
      }
    }
    auto end = std::chrono::high_resolution_clock::now();
    elapsed("hit", end, start);
  }
  {
    auto start = std::chrono::high_resolution_clock::now();
    for (auto m = 0; m < miss_count; ++m)
    {
      for (auto n = 0; n < populate_count; ++n)
      {
        cnt += is_set(s, rnd(generator)) ? 1 : 0;
      }
    }
    auto end = std::chrono::high_resolution_clock::now();
    elapsed("miss", end, start);
  }
  std::cout << "hit count: " << cnt << std::endl;
}

struct cmp_by_length {
  template<class T>
  bool operator()(T const &a, T const &b) const {
    return a.length() < b.length() || (a.length() == b.length() && a < b);
  }
};

struct option : public std::set<std::string, cmp_by_length>
{
  using std::set<std::string, cmp_by_length>::set;
  void help() const
  {
    auto first = true;
    for (const auto& s : *this)
    {
      std::cout << (first ? " " : " | ") << s;
      first = false;
    }
    std::cout << std::endl;
  }
  const std::string& name() const
  {
    return *rbegin();
  }
  bool contains(const std::string& s) const
  {
    auto ret = count(s) == 1;
    if (ret)
    {
      test_name = name();
    }
    return ret;
  }
};

int main(int argc, char** argv)
{
  try
  {
    option _std = { "s", "set", "std::set" };
    option unordered = { "u", "unordered", "unordered_set", "std::unordered_set" };
    option btree = { "b", "btree", "btree_set", "btree::btree_set" };
    option sparse = { "sp", "sparse", "sparse_hash_set", "google::sparse_hash_set" };
    option dense = { "d", "dense", "dense_hash_set", "google::dense_hash_set" };
    option closed = { "c", "closed", "closed_hash_set", "mct::closed_hash_set" };
    option forward = { "f", "forward", "forward_hash_set", "mct::forward_hash_set" };
    option huge_forward = { "hf", "huge_forward", "huge_forward_hash_set", "mct::huge_forward_hash_set" };
    option huge_linked = { "hl", "huge_linked", "huge_linked_hash_set", "mct::huge_linked_hash_set" };
    option bit_vector = { "bv", "bit_vector", "sdsl::bit_vector" };
    option ewah = { "ewah", "EWAHBoolArray" };
    option ewah64 = { "ewah64", "EWAHBoolArray64" };
    option _sparse_set = { "ss", "sparse_set" };
    option _unordered_sparse_set = { "uss", "unordered_sparse_set" };
    option concise_set = { "cs", "concise_set" };
    if (argc == 1)
    {
      std::cout << "Usage: test [<cnt>] [hit <multiplier>] [miss <multiplier>] <type> " << std::endl;
      _std.help();
      unordered.help();
      btree.help();
      sparse.help();
      dense.help();
      closed.help();
      forward.help();
      huge_forward.help();
      huge_linked.help();
      bit_vector.help();
      ewah.help();
      ewah64.help();
      _sparse_set.help();
      _unordered_sparse_set.help();
      concise_set.help();
      std::cout << " cnt - number of values in set, default: " << amount << std::endl;
      std::cout << " pop_hit cnt  - number of population hit steps, default: 0" << std::endl;
      std::cout << " hit cnt  - number of hit steps, default: 0" << std::endl;
      std::cout << " miss cnt - number of miss steps, default: 0" << std::endl;
      std::cout << " max_val cnt - max value in sequence" << std::endl;
      std::cout << " max_bit cnt - max value in sequence = 2^max_bit - 1" << std::endl;
      return 0;
    }
    enum CntType { Cnt, PopHit, Hit, Miss, MaxVal, MaxBit } cntType = Cnt;
    for (int i = 1; i < argc; ++i)
    {
      std::string s(argv[i]);
      if (_std.contains(s))
      {
        test<std::set<test_t, std::less<test_t>, PoolAllocator<test_t>>>();
      }
      else if (unordered.contains(s))
      {
        test<std::unordered_set<test_t, std::hash<test_t>, std::equal_to<test_t>, PoolAllocator<test_t>>>();
      }
      else if (btree.contains(s))
      {
        test<btree::btree_set<test_t, std::less<test_t>, PoolAllocator<test_t>>>();
      }
      else if (sparse.contains(s))
      {
        test<google::sparse_hash_set<test_t, std::hash<test_t>, std::equal_to<test_t>, Reallocator<test_t>>>();
      }
      else if (dense.contains(s))
      {
        test<Dense>();
      }
      else if (closed.contains(s))
      {
        test<mct::closed_hash_set<test_t, std::hash<test_t>, std::equal_to<test_t>, PoolAllocator<test_t>>>();
      }
      else if (forward.contains(s))
      {
        test<mct::forward_hash_set<test_t, std::hash<test_t>, std::equal_to<test_t>, PoolAllocator<test_t>>>();
      }
      else if (huge_forward.contains(s))
      {
        test<mct::huge_forward_hash_set<test_t, std::hash<test_t>, std::equal_to<test_t>, PoolAllocator<test_t>>>();
      }
      else if (huge_linked.contains(s))
      {
        test<mct::huge_linked_hash_set<test_t, std::hash<test_t>, std::equal_to<test_t>, PoolAllocator<test_t>>>();
      }
      else if (bit_vector.contains(s))
      {
        test<sdsl::bit_vector>();
      }
      else if (ewah.contains(s))
      {
        test<EWAHBoolArray<>>();
      }
      else if (ewah64.contains(s))
      {
        test<EWAHBoolArray<uint64_t>>();
      }
      else if (_sparse_set.contains(s))
      {
        test<sparse_set>();
      }
      else if (_unordered_sparse_set.contains(s))
      {
        test<unordered_sparse_set>();
      }
      else if (concise_set.contains(s))
      {
        test<ConciseSet<>>();
      }
      else if (s == "pop_hit")
      {
        cntType = PopHit;
      }
      else if (s == "hit")
      {
        cntType = Hit;
      }
      else if (s == "miss")
      {
        cntType = Miss;
      }
      else if (s == "max_val")
      {
        cntType = MaxVal;
      }
      else if (s == "max_bit")
      {
        cntType = MaxBit;
      }
      else
      {
        auto n = std::stoull(s);
        switch (cntType)
        {
        case Cnt: populate_count = n; break;
        case PopHit: pop_hit_count = n; break;
        case Hit: hit_count = n; break;
        case Miss: miss_count = n; break;
        case MaxVal: max_value = static_cast<test_t>(n); break;
        case MaxBit: max_value = static_cast<test_t>(n > 63 ? std::numeric_limits<uint64_t>::max() : (1 << n) - 1); break;
        }
        cntType = Cnt;
      }
    }
  }
  catch(const std::exception& e)
  {
    std::cout << e.what() << std::endl;
  }
  return 0;
}
