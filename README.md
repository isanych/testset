# Test different sets performance
std::set, std::unordered_set, btree::btree_set (https://code.google.com/archive/p/cpp-btree/), google::sparse_hash_set & google::dense_hash_set (https://github.com/sparsehash/sparsehash)

Visual Studio 2015 update 3, 64 bit binary, 20,000,000 of unique randomly distributed uint64_t values (153MB of data) on i7-6700 CPU.

### Memory usage (allocated memory / size of data):

set | ratio
--- | ---
btree::btree_set | 1.31
google::sparse_hash_set | 2.31
std::set | 5.00
std::set pool | 5.01
google::dense_hash_set | 6.29
mct::closed_hash_set | 6.50
mct::forward_hash_set | 6.50
mct::huge_forward_hash_set | 6.50
std::unordered_set | 9.29
std::unordered_set pool | 9.71
mct::huge_linked_hash_set | 9.86

### Add new value to set (Time / Best Time):
set | ratio
--- | ---
google::dense_hash_set | 1.00
mct::closed_hash_set | 1.03
mct::huge_forward_hash_set | 1.88
mct::forward_hash_set | 1.89
mct::huge_linked_hash_set | 2.12
btree::btree_set | 2.32
std::unordered_set pool | 2.58
std::unordered_set | 2.88
google::sparse_hash_set | 3.53
std::set pool | 7.63
std::set | 8.22

### Add existing value to set (Time / Best Time):
set | ratio
--- | ---
google::dense_hash_set | 1.00
mct::forward_hash_set | 1.04
mct::huge_forward_hash_set | 1.06
mct::closed_hash_set | 1.09
mct::huge_linked_hash_set | 1.12
google::sparse_hash_set | 1.72
std::unordered_set pool | 2.03
std::unordered_set | 2.03
btree::btree_set | 2.66
std::set pool | 10.55
std::set | 10.55

There is no advantage in usage of `std::set` or `std::unordered_set` unless your data set is very small. `std::set` is very slow, `std::unordered_set` is not the fastest and uses too much memory (and memory pool does not help them much).<BR>
`google::dense_hash_set` is the fastest implementation but it requires you to set aside one key value.<BR>
`mct::closed_hash_set` is close to `google::dense_hash_set` by performance and memory consumption and does not have limitations on values in set.<BR>
`btree::btree_set` has very small memory overhead and decent performance in comparison with `std::unordered_set`, but slower than fastest implementations.<BR>
I’ve tried to use compressed & sparse bitsets, but have not found implementations which could handle full range of 64 bit values and even on smaller ranges all implementations which I’ve tried have performance issues with multiple inserts.
