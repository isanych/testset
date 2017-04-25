# Test different sets performance
std::set, std::unordered_set, btree::btree_set (https://code.google.com/archive/p/cpp-btree/), google::sparse_hash_set & google::dense_hash_set (https://github.com/sparsehash/sparsehash)

Visual Studio 2015 update 3, 64 bit binary, 20,000,000 of unique randomly distributed uint64_t values (153MB of data) on i7-6700 CPU.

### Memory usage (allocated memory / size of data):
Overhead of memory manager is not included.

set | ratio
--- | ---
btree::btree_set | 1.31
google::sparse_hash_set | 2.31
std::set | 5.00
std::set pool | 5.01
boost::unordered_set | 5.36
boost::unordered_set pool | 5.52
google::dense_hash_set | 6.29
mct::closed_hash_set | 6.50
mct::huge_forward_hash_set | 6.50
mct::forward_hash_set | 6.50
std::unordered_set | 9.29
std::unordered_set pool | 9.71
spp::sparse_hash_set | 9.73
mct::huge_linked_hash_set | 9.86
spp::sparse_hash_set pool | 10.92

### Add new value to set (Time / Best Time):
set | ratio
--- | ---
google::dense_hash_set | 1.00
mct::closed_hash_set | 1.01
mct::huge_forward_hash_set | 1.85
mct::forward_hash_set | 1.85
spp::sparse_hash_set pool | 1.86
mct::huge_linked_hash_set | 2.02
spp::sparse_hash_set | 2.30
btree::btree_set | 2.31
std::unordered_set pool | 2.53
boost::unordered_set pool | 2.73
std::unordered_set | 2.84
boost::unordered_set | 3.02
google::sparse_hash_set | 3.52
std::set pool | 7.37
std::set | 7.96

### Add existing value to set (Time / Best Time):
set | ratio
--- | ---
google::dense_hash_set | 1.00
mct::huge_forward_hash_set | 1.05
mct::forward_hash_set | 1.06
mct::closed_hash_set | 1.09
mct::huge_linked_hash_set | 1.10
spp::sparse_hash_set | 1.65
google::sparse_hash_set | 1.74
spp::sparse_hash_set pool | 1.75
std::unordered_set pool | 1.82
boost::unordered_set pool | 1.88
boost::unordered_set | 1.99
std::unordered_set | 2.00
btree::btree_set | 2.73
std::set pool | 9.85
std::set | 10.36

There is no advantage in usage of `std::set` or `std::unordered_set` unless your data set is very small. `std::set` is very slow, `std::unordered_set` is not the fastest and uses too much memory (and memory pool does not help them much).<BR>
`google::dense_hash_set` is the fastest implementation but it requires you to set aside one key value.<BR>
`mct::closed_hash_set` is close to `google::dense_hash_set` by performance and memory consumption and does not have limitations on values in set.<BR>
`btree::btree_set` has very small memory overhead and decent performance in comparison with `std::unordered_set`, but slower than fastest implementations.<BR>
I’ve tried to use compressed & sparse bitsets, but have not found implementations which could handle full range of 64 bit values and even on smaller ranges all implementations which I’ve tried have performance issues with multiple inserts.
