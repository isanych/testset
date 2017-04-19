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

#ifdef _DEBUG
const auto amount = 100000;
#else
const auto amount = 10000000;
#endif
const auto mem_offset = sizeof(size_t);

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
    free(p);
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
    auto used = _alloc.bytes - _free.bytes;
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


typedef google::dense_hash_set<int64_t, std::hash<int64_t>, std::equal_to<int64_t>, Reallocator<int64_t>> Dense;

size_t populate_count = amount;
size_t hit_count = 0;
size_t miss_count = 0;

template<typename T>
void init_set(T& s){}

template<>
void init_set(Dense& s)
{
  s.set_empty_key(0);
}

template<typename T>
void elapsed(const char* name, T end, T start)
{
  std::cout << name << ", sec: " << std::fixed << std::setprecision(2) <<
    std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.0 << std::endl;
}

std::string test_name;

template<typename T>
void test()
{
  std::cout << test_name << std::endl;
  std::uniform_int_distribution<int64_t> rnd(std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max());
  T s;
  init_set(s);
  std::default_random_engine generator(5489);
  auto population_start = std::chrono::high_resolution_clock::now();
  for (auto n = 0; n < populate_count; ++n)
  {
    s.insert(rnd(generator));
  }
  auto population_end = std::chrono::high_resolution_clock::now();
  pool.report(s.size() * sizeof(int64_t));
  elapsed("population", population_end, population_start);
  size_t cnt = 0;
  auto hit_start = std::chrono::high_resolution_clock::now();
  for (auto h = 0; h < hit_count; ++h)
  {
    std::default_random_engine hit_generator(5489);
    for (auto n = 0; n < populate_count; ++n)
    {
      cnt += s.find(rnd(hit_generator)) != s.end() ? 1 : 0;
    }
  }
  auto hit_end = std::chrono::high_resolution_clock::now();
  elapsed("hit", hit_end, hit_start);
  auto miss_start = std::chrono::high_resolution_clock::now();
  for (auto m = 0; m < miss_count; ++m)
  {
    for (auto n = 0; n < populate_count; ++n)
    {
      cnt += s.find(rnd(generator)) != s.end() ? 1 : 0;
    }
  }
  auto miss_end = std::chrono::high_resolution_clock::now();
  elapsed("miss", miss_end, miss_start);
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
      std::cout << " cnt - number of values in set, default: " << amount << std::endl;
      std::cout << " hit cnt  - number of hit steps, default: 0" << std::endl;
      std::cout << " miss cnt - number of miss steps, default: 0" << std::endl;
      return 0;
    }
    enum CntType { Cnt, Hit, Miss } cntType = Cnt;
    for (int i = 1; i < argc; ++i)
    {
      std::string s(argv[i]);
      if (_std.contains(s))
      {
        test<std::set<int64_t, std::less<int64_t>, PoolAllocator<int64_t>>>();
      }
      else if (unordered.contains(s))
      {
        test<std::unordered_set<int64_t, std::hash<int64_t>, std::equal_to<int64_t>, PoolAllocator<int64_t>>>();
      }
      else if (btree.contains(s))
      {
        test<btree::btree_set<int64_t, std::less<int64_t>, PoolAllocator<int64_t>>>();
      }
      else if (sparse.contains(s))
      {
        test<google::sparse_hash_set<int64_t, std::hash<int64_t>, std::equal_to<int64_t>, Reallocator<int64_t>>>();
      }
      else if (dense.contains(s))
      {
        test<Dense>();
      }
      else if (closed.contains(s))
      {
        test<mct::closed_hash_set<int64_t, std::hash<int64_t>, std::equal_to<int64_t>, PoolAllocator<int64_t>>>();
      }
      else if (forward.contains(s))
      {
        test<mct::forward_hash_set<int64_t, std::hash<int64_t>, std::equal_to<int64_t>, PoolAllocator<int64_t>>>();
      }
      else if (huge_forward.contains(s))
      {
        test<mct::huge_forward_hash_set<int64_t, std::hash<int64_t>, std::equal_to<int64_t>, PoolAllocator<int64_t>>>();
      }
      else if (huge_linked.contains(s))
      {
        test<mct::huge_linked_hash_set<int64_t, std::hash<int64_t>, std::equal_to<int64_t>, PoolAllocator<int64_t>>>();
      }
      else if (s == "hit")
      {
        cntType = Hit;
      }
      else if (s == "miss")
      {
        cntType = Miss;
      }
      else
      {
        auto n = std::stoull(s);
        switch (cntType)
        {
        case Cnt: populate_count = n; break;
        case Hit: hit_count = n; break;
        case Miss: miss_count = n; break;
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
