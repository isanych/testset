# Test different sets performance
std::set, std::unordered_set, btree::btree_set (https://code.google.com/archive/p/cpp-btree/), google::sparse_hash_set & google::dense_hash_set (https://github.com/sparsehash/sparsehash)

Visual Studio 2015 update 3, 64 bit binary, 10,000,000 of random int64_t.

### Memory usage (working set / size of data):

set | ratio
--- | ---
btree::btree_set | 1.45
google::sparse_hash_set | 1.71
google::dense_hash_set | 5.10
std::set | 6.22
std::unordered_set | 7.44

### Population with unique randonly distributed values (Time / Best Time):
set | ratio
--- | ---
google::dense_hash_set | 1.00
btree::btree_set | 1.86
std::unordered_set | 2.94
google::sparse_hash_set | 3.24
std::set | 7.39

### Search with unique randonly distributed values (Time / Best Time):
set | ratio
--- | ---
google::dense_hash_set | 1.00
std::unordered_set | 1.50
google::sparse_hash_set | 1.71
btree::btree_set | 2.46
std::set | 8.98
