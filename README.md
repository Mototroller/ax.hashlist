# ax.hashlist [![Build Status](https://travis-ci.org/Mototroller/ax.hashlist.svg?branch=master)](https://travis-ci.org/Mototroller/ax.hashlist)

Flat fixed size STL-compatible container, hybrid of `std::(array-list-map)`. Stores `std::pair<const key, value>`, allows iterating in adding order, finding by key and erasing with constant complexity.

* Flat (`standard_layout` if `key` and `value` are `standard_layout`)
* Fixed size (same as `std::array`)
* Navigation and finding based on offsets
  * ...so it can be mmap'ed and **shared between** threads and **processes**
* Cache- and branch- friendly with some aditional tuning
* Complexity depends on load factor, best performance while rarefied
* Under hood: doubly linked list with sentinel + open addressing hash table

| Action | Complexity on average  | ...and worst case |
| ------------- |:-------------:|:-----:|
| `find(key)` | **O(1)** | O(N) |
| `emplace_back`/`push_back` | **O(1)** | O(N) |
| `erace(it)` | O(1) | O(1) |
| iterating (`++it`/`--it`) | O(1) | O(1) |

To be continued...

### TODO:

* performance measurements
* implement assignment operator
* implement standard `emplace_back` overloadings (see `std::map`)
* advanced hashing strategies (default one is a little bit overweight and acceptable for literal type keys only)
* do small changes to satisfy `std::unordered_multimap`'s behavior
