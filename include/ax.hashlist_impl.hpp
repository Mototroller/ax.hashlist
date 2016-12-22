#include <ax.hashlist.hpp>

namespace ax { namespace hl {
    
    template <typename keyT>
    uint64_t FNV_1<keyT>::h1(keyT k) {
        uint64_t hash = 0xcbf29ce484222325UL;
        auto bytes = reinterpret_cast<unsigned char const*>(&k);
        for(uint8_t byte = 0; byte < sizeof(keyT); ++byte) {
            hash ^= static_cast<uint64_t>(bytes[byte]);
            hash *= 0x100000001b3UL;
        }
        return hash;
    }
    
    template <typename keyT>
    uint32_t FNV_1<keyT>::h2(keyT k) {
        uint32_t hash = 2166136261U;
        auto bytes = reinterpret_cast<unsigned char const*>(&k);
        for(uint8_t byte = 0; byte < sizeof(keyT); ++byte) {
            hash ^= static_cast<uint32_t>(bytes[byte]);
            hash *= 16777619U;
        }
        hash |= 0x1; // !!! for being odd
        return hash;
    }
    
    template <typename K, typename O, size_t S, class H>
    template <typename... Args>
    auto hashlist<K,O,S,H>::
    emplace_back(key_type const& key, Args&&... args) -> iterator {
        auto H1 = hasher::h1(key);
        auto H2 = hasher::h2(key);
        auto& sentinel = cells_[0];
        auto& h = header_;
        auto& cs = cells_;
        for(size_t i = 0; i < SIZE; ++i) {
            size_t bit = (H1 + i*H2) % SIZE;
            if(!h[bit]) {
                size_t idx = 1 + bit;
                auto& inserted = cs[idx];
                new(&inserted.value()) value_type(key, std::forward<Args>(args)...);
                h.flip(bit);
                
                offset_t sidx(idx);
                inserted.next_offset = -sidx;
                inserted.prev_offset = sentinel.prev_offset - sidx;
                cs[sentinel.prev_offset].next_offset = -inserted.prev_offset;
                sentinel.prev_offset = sidx;
                return --end();
            }
        }
        throw std::bad_alloc{};
    }
    
    template <typename K, typename O, size_t S, class H>
    auto hashlist<K,O,S,H>::
    find_cell(key_type const& key) const -> cell_t const* {
        auto H1 = hasher::h1(key);
        auto H2 = hasher::h2(key);
        auto const& h = header_;
        auto const& cs = cells_;
        for(size_t i = 0; i < SIZE; ++i) {
            size_t bit = (H1 + i*H2) % SIZE;
            size_t idx = 1 + bit;
            auto const& c = cs[idx];
            if(h[bit] && c.value().first == key)
                return &c;
        }
        return &cs[0];
    }
    
    template <typename K, typename O, size_t S, class H>
    auto hashlist<K,O,S,H>::
    remove_cell(cell_t* cell) -> cell_t* {
        auto& cs = cells_;
        size_t idx = cell - cs.begin();
        size_t bit = idx - 1;
        
        header_[bit] = false;
        cell->value().~value_type();
        
        (cell + cell->prev_offset)->next_offset += cell->next_offset;
        (cell + cell->next_offset)->prev_offset += cell->prev_offset;
        return (cell + cell->next_offset);
    }
    
} // hl
} // ax
