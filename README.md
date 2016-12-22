# ax.hashlist [![Build Status](https://travis-ci.org/Mototroller/ax.hashlist.svg?branch=master)](https://travis-ci.org/Mototroller/ax.hashlist)

Flat fixed size STL-compatible container, hybrid of `std::(array-list-unordered_multimap)`. Stores `std::pair<const key, value>`, allows iterating in adding order (and reversed), finding by key and erasing with constant complexity.

* Flat (`standard_layout` if `key` and `value` are `standard_layout`)
* Fixed size (same as `std::array`)
* Navigation and finding based on offsets
  * ...so it can be mmap'ed and **shared between** threads and **processes**
* Cache- and branch- friendly with some aditional tuning
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

To be continued...

### TODO:

* performance measurements
* implement assignment operator
* try to emulate standard `emplace_back` behaviour (`std::map`'s' way is unacceptable due to fixed storage)
