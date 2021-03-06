# ax.hashlist [![Build Status](https://travis-ci.org/Mototroller/ax.hashlist.svg?branch=master)](https://travis-ci.org/Mototroller/ax.hashlist)

Flat fixed size STL-compatible container, hybrid of `std::(array-list-unordered_multimap)`. Stores `std::pair<const key, value>`, allows iterating in adding order (and reversed), finding by key and erasing with constant complexity.

* Flat (`standard_layout` if `key` and `value` are `standard_layout`)
* Fixed size (same as `std::array`)
* Navigation and finding based on offsets
  * ...so it can be mmap'ed and **shared between** threads and **processes**
* Cache- and branch- friendly with some additional tuning
* Complexity depends on load factor and Hasher tuning, best performance while rarefied
* Under hood: doubly linked list with sentinel + open addressing hash table (uses double hashing)

| Action | Complexity on average  | ...and worst case |
| ------------- |:-------------:|:-----:|
| `find(key)` | **O(1)** | O(N) |
| `emplace_back`/`push_back` | **O(1)** | O(N) |
| `erace(it)` | O(1) | O(1) |
| iterating (`++it`/`--it`) | O(1) | O(1) |

For now 2 default hashing strategies are provided:
* tuned `FNV_1` for general purposes
* `Incremental_integer_fasthash` for integer mostly incremental keys

### Performance (beta):

Test machine:

```
model name      : Intel(R) Core(TM) i7-4700MQ CPU @ 2.40GHz
cache size      : 6144 KB
cpu cores       : 4
cache_alignment : 64
```

```cpp
struct dummy_t {
    std::array<char, 32> data;
    dummy_t(char c) :
        data{42} {
        volatile char cv = c;
        data[0] = cv;
        data[data.size()-1] = cv;
    };
};
using hl_t = hl::hashlist<size_t, dummy_t, 1024>;
```

![Alt emplace](https://rawgit.com/Mototroller/ax.hashlist/master/emplace.svg)

![Alt find](https://rawgit.com/Mototroller/ax.hashlist/master/find.svg)

### TODO:

* performance measurements (extend)
* implement assignment operator
* try to emulate standard `emplace_back` behaviour (`std::map`'s way is unacceptable due to fixed storage)
