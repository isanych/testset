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
size_t hit_count = 0;
size_t miss_count = 0;

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
constexpr bool operator!=(const PoolAllocator<T> &, const PoolAllocator<U> &) noexcept { return false; }

template<typename T>
void elapsed(const char* name, T end, T start)
{
  std::cout << name << ", sec: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.0 << std::endl;
}

template<typename T>
void test(const std::string& name)
{
  std::uniform_int_distribution<int64_t> rnd(std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max());
  T s;
  init_set(s);
  std::default_random_engine generator(5489);
  reset();
  auto population_start = std::chrono::high_resolution_clock::now();
  for (auto n = 0; n < populate_count; ++n)
  {
    s.insert(rnd(generator));
  }
  auto population_end = std::chrono::high_resolution_clock::now();
  auto nc = new_counter;
  auto dc = del_counter;
  auto memory_size = reset();
  auto data_size = s.size() * sizeof(int64_t);
  std::cout << name << ": " << data_size / 1024 << " KB of data " << memory_size / 1024 << " KB allocated, ratio " << std::setprecision(2) << static_cast<double>(memory_size) / data_size << " new cnt: " << nc << " del cnt: "<< dc << std::endl;
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
  bool contains(const std::string& s) const { return count(s) == 1; }
};

int main(int argc, char** argv)
{
  try
  {
    option _std = { "s", "set", "std::set" };
    option unordered = { "u", "unordered", "unordered_set", "std::unordered_set" };
    option unordered_pool = { "up", "unordered with pool", "unordered_set with pool", "std::unordered_set with pool" };
    option btree = { "b", "btree", "btree_set", "btree::btree_set" };
    option sparse = { "sp", "sparse", "sparse_hash_set", "google::sparse_hash_set" };
    option sparse_pool = { "spp", "sparse with pool", "sparse_hash_set with pool", "google::sparse_hash_set with pool" };
    option dense = { "d", "dense", "dense_hash_set", "google::dense_hash_set" };
    option dense_pool = { "dp", "dense with pool", "dense_hash_set with pool", "google::dense_hash_set with pool" };
    option closed = { "c", "closed", "closed_hash_set", "mct::closed_hash_set" };
    option forward = { "f", "forward", "forward_hash_set", "mct::forward_hash_set" };
    option huge_forward = { "hf", "huge_forward", "huge_forward_hash_set", "mct::huge_forward_hash_set" };
    option huge_linked = { "hl", "huge_linked", "huge_linked_hash_set", "mct::huge_linked_hash_set" };
    if (argc == 1)
    {
      std::cout << "Usage: test [<cnt>] [hit <multiplier>] [miss <multiplier>] <type> " << std::endl;
      _std.help();
      unordered.help();
      unordered_pool.help();
      btree.help();
      sparse.help();
      sparse_pool.help();
      dense.help();
      dense_pool.help();
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
        test<std::set<int64_t>>(_std.name());
      }
      else if (unordered.contains(s))
      {
        test<std::unordered_set<int64_t>>(unordered.name());
      }
      else if (unordered_pool.contains(s))
      {
        test<std::unordered_set<int64_t, std::hash<int64_t>, std::equal_to<int64_t>, PoolAllocator<int64_t>>>(unordered_pool.name());
      }
      else if (btree.contains(s))
      {
        test<btree::btree_set<int64_t>>(btree.name());
      }
      else if (sparse.contains(s))
      {
        test<google::sparse_hash_set<int64_t>>(sparse.name());
      }
      else if (sparse_pool.contains(s))
      {
        test<google::sparse_hash_set<int64_t, std::hash<int64_t>, std::equal_to<int64_t>, PoolAllocator<int64_t>>>(sparse_pool.name());
      }
      else if (dense.contains(s))
      {
        test<google::dense_hash_set<int64_t>>(dense.name());
      }
      else if (dense_pool.contains(s))
      {
        test<google::dense_hash_set<int64_t, std::hash<int64_t>, std::equal_to<int64_t>, PoolAllocator<int64_t>>>(dense_pool.name());
      }
      else if (closed.contains(s))
      {
        test<mct::closed_hash_set<int64_t>>(closed.name());
      }
      else if (forward.contains(s))
      {
        test<mct::forward_hash_set<int64_t>>(forward.name());
      }
      else if (huge_forward.contains(s))
      {
        test<mct::huge_forward_hash_set<int64_t>>(huge_forward.name());
      }
      else if (huge_linked.contains(s))
      {
        test<mct::huge_linked_hash_set<int64_t>>(huge_linked.name());
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
