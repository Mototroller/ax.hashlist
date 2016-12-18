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
    template <typename D, typename V>
    hashlist<K,O,S,H>::iterator_base<D,V>::
    iterator_base(D ptr) : ptr_(ptr) {}
    
    template <typename K, typename O, size_t S, class H>
    template <typename D, typename V>
    template <typename d, typename v, typename temp>
    hashlist<K,O,S,H>::iterator_base<D,V>::
    iterator_base(iterator_base<d,v> const& other) :
        iterator_base(other.ptr_) {}
    
    template <typename K, typename O, size_t S, class H>
    template <typename D, typename V>
    auto hashlist<K,O,S,H>::iterator_base<D,V>::
    swap(iterator_base& other) noexcept -> void {
        std::swap(ptr_, other.ptr_); }
    
    template <typename K, typename O, size_t S, class H>
    template <typename D, typename V>
    auto hashlist<K,O,S,H>::iterator_base<D,V>::
    operator*() const -> typename it::reference {
        return ptr_->value(); }
    
    template <typename K, typename O, size_t S, class H>
    template <typename D, typename V>
    auto hashlist<K,O,S,H>::iterator_base<D,V>::
    operator->() const -> typename it::pointer {
        return &(this->operator*()); }
    
    template <typename K, typename O, size_t S, class H>
    template <typename D, typename V>
    auto hashlist<K,O,S,H>::iterator_base<D,V>::
    operator++() -> iterator_base& {
        ptr_ += ptr_->next_offset;
        return *this;
    }
    
    template <typename K, typename O, size_t S, class H>
    template <typename D, typename V>
    auto hashlist<K,O,S,H>::iterator_base<D,V>::
    operator==(iterator_base const& other) const -> const bool {
        return ptr_ == other.ptr_; }
    
    template <typename K, typename O, size_t S, class H>
    template <typename D, typename V>
    auto hashlist<K,O,S,H>::iterator_base<D,V>::
    operator!=(iterator_base const& other) const -> const bool {
        return !this->operator==(other); }
    
    template <typename K, typename O, size_t S, class H>
    template <typename D, typename V>
    auto hashlist<K,O,S,H>::iterator_base<D,V>::
    operator++(int) -> iterator_base {
        iterator_base old(*this);
        this->operator++();
        return old;
    }
    
    template <typename K, typename O, size_t S, class H>
    template <typename D, typename V>
    auto hashlist<K,O,S,H>::iterator_base<D,V>::
    operator--() -> iterator_base& {
        ptr_ += ptr_->prev_offset;
        return *this;
    }
    
    template <typename K, typename O, size_t S, class H>
    template <typename D, typename V>
    auto hashlist<K,O,S,H>::iterator_base<D,V>::
    operator--(int) -> iterator_base {
        iterator_base old(*this);
        this->operator--();
        return old;
    }
    
    template <typename K, typename O, size_t S, class H>
    hashlist<K,O,S,H>::
    hashlist() :
        header_(0x0) {
        cells_[0].next_offset = 0;
        cells_[0].prev_offset = 0;
    }
    
    template <typename K, typename O, size_t S, class H>
    hashlist<K,O,S,H>::
    ~hashlist() {
        clear(); }
    
    template <typename K, typename O, size_t S, class H>
    auto hashlist<K,O,S,H>::
    size() const -> const size_type {
        return header_.count(); }
    
    template <typename K, typename O, size_t S, class H>
    constexpr auto hashlist<K,O,S,H>::
    max_size() -> const size_type {
        return SIZE - 1; }
    
    template <typename K, typename O, size_t S, class H>
    auto hashlist<K,O,S,H>::
    load_factor() const -> const float {
        return size() / max_size(); }
    
    template <typename K, typename O, size_t S, class H>
    auto hashlist<K,O,S,H>::
    empty() const -> const bool {
        return cells_[0].next_offset == 0; }
    
    template <typename K, typename O, size_t S, class H>
    auto hashlist<K,O,S,H>::
    clear() -> void {
        for(auto i = begin(), e = end(); i != e; i = erase(i)); }
    
    template <typename K, typename O, size_t S, class H>
    auto hashlist<K,O,S,H>::
    get_sentinel() const -> mapped_type const& {
        return cells_[0].value().second; }
    
    template <typename K, typename O, size_t S, class H>
    auto hashlist<K,O,S,H>::
    get_sentinel() -> mapped_type& {
        return const_cast<mapped_type&>(static_cast<hashlist const*>(this)->get_sentinel()); }
    
    template <typename K, typename O, size_t S, class H>
    template <typename... Args>
    auto hashlist<K,O,S,H>::
    emplace_back(key_type const& key, Args&&... args) -> void {
        auto H1 = hasher::h1(key);
        auto H2 = hasher::h2(key);
        auto& sentinel = cells_[0];
        auto& h = header_;
        auto& cs = cells_;
        for(size_t i = 0; i < SIZE; ++i) {
            size_t idx = (H1 + i*H2) % SIZE;
            if(idx != 0 && h[idx] == false) {
                h.flip(idx);
                auto& inserted = cs[idx];
                new(&inserted.value()) value_type(key, std::forward<Args>(args)...);
                offset_t sidx(idx);
                inserted.next_offset = -sidx;
                inserted.prev_offset = sentinel.prev_offset - sidx;
                cs[sentinel.prev_offset].next_offset = -inserted.prev_offset;
                sentinel.prev_offset = sidx;
                return;
            }
        }
        throw std::bad_alloc{};
    }
    
    template <typename K, typename O, size_t S, class H>
    auto hashlist<K,O,S,H>::
    push_back(const_reference value) -> void {
        emplace_back(value.first, value.second); }
    
    template <typename K, typename O, size_t S, class H>
    auto hashlist<K,O,S,H>::
    find(key_type const& key) const -> const_iterator {
        return const_iterator(find_cell(key)); }
    
    template <typename K, typename O, size_t S, class H>
    auto hashlist<K,O,S,H>::
    find(key_type const& key) -> iterator {
        return iterator(const_cast<cell_t*>(find_cell(key))); }
    
    template <typename K, typename O, size_t S, class H>
    auto hashlist<K,O,S,H>::
    erase(const_iterator pos) -> iterator {
        return iterator(remove_cell(const_cast<cell_t*>(pos.ptr_))); }
    
    template <typename K, typename O, size_t S, class H>
    auto hashlist<K,O,S,H>::
    offset_of_element(const_iterator const& citer) const -> const size_t {
        return citer.ptr_ - &(cells_[0]); }
    
    template <typename K, typename O, size_t S, class H>
    auto hashlist<K,O,S,H>::
    element_by_offset(size_t idx) -> iterator {
        return iterator(&cells_[idx]); }
    
    template <typename K, typename O, size_t S, class H>
    auto hashlist<K,O,S,H>::cell_t::
    value() const -> value_type const& {
        return *reinterpret_cast<const value_type*>(&value_); }
    
    template <typename K, typename O, size_t S, class H>
    auto hashlist<K,O,S,H>::cell_t::
    value() -> value_type& {
        return const_cast<value_type&>(static_cast<cell_t const*>(this)->value()); }
    
    template <typename K, typename O, size_t S, class H>
    auto hashlist<K,O,S,H>::
    find_cell(key_type const& key) const -> cell_t const* {
        auto H1 = hasher::h1(key);
        auto H2 = hasher::h2(key);
        auto const& h = header_;
        auto const& cs = cells_;
        for(size_t i = 0; i < SIZE; ++i) {
            size_t idx = (H1 + i*H2) % SIZE;
            auto const& c = cs[idx];
            if(h[idx] == true && c.value().first == key)
                return &c;
        }
        return &cs[0];
    }
    
    template <typename K, typename O, size_t S, class H>
    auto hashlist<K,O,S,H>::
    remove_cell(cell_t* cell) -> cell_t* {
        auto& cs = cells_;
        offset_t sidx = cell - cs.begin();
        
        header_[sidx] = false;
        cell->value().~value_type();
        
        (cell + cell->prev_offset)->next_offset += cell->next_offset;
        (cell + cell->next_offset)->prev_offset += cell->prev_offset;
        return (cell + cell->next_offset);
    }
    
} // hl
} // ax