# ax.hashlist [![Build Status](https://travis-ci.org/Mototroller/ax.hashlist.svg?branch=master)](https://travis-ci.org/Mototroller/ax.hashlist)

Flat fixed size STL-compatible container, hybrid of std::(array-list-map). Stores `std::pair<const key, value>`, allows iterating in adding order and finding by key with constant complexity.
* Fixed size (same as `std::array`)
* Flat (`standard_layout` if `key` and `value` are `standard_layout`)
* Navigation and finding based on offsets
  * ...so it can be mmap'ed and **shared between** threads and **processes**
* Cache- and branch- friendly with some aditional tuning
* Complexity depends on load factor, best performance while rarefied

| Action | Complexity on average  | ...and worst case |
| ------------- |:-------------:|:-----:|
| `find` | **O(1)** | O(N) |
| `emplace_back`/`push_back` | **O(1)** | O(N) |
| iterating (`++it`/`--it`) | O(1) | O(1) |

To be continued...

**TODO:** performance measurements, move spaghetti definitions to base header (?)
