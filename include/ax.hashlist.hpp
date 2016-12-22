#pragma once

#include <algorithm>
#include <array>
#include <bitset>
#include <climits>
#include <cstdint>
#include <iterator>
#include <utility>

#define LOG_HEAD "[hl]: "

namespace ax { namespace hl {
    
    /**
     * Special default hash policy for haslist.
     * Contains FNV-1 64- and 32-bits implementations,
     * Function h2 (FNV32-1) returns odd values only.
     * TODO: auto compile-time tuning
     */
    template <typename keyT>
    struct FNV_1 {
        /// 64-bit FNV-1
        inline static uint64_t h1(keyT k);
        
        /// 32-bit FNV-1
        inline static uint32_t h2(keyT k);
    };
    
    /**
     * Special hash policy for mostly incremental integer keys.
     * Objects will be stored almost continuously.
     */
    template <typename keyT>
    struct Incremental_integer_fasthash {
        
        static_assert(sizeof(keyT) <= sizeof(uint64_t), LOG_HEAD
            "To use Incremental_integer_fasthash keyT must be less than sizeof(uint64_t)");
        
        inline static uint64_t h1(keyT k) {
            return uint64_t(k); }
        
        inline static uint32_t h2(keyT k) {
            return 1; }
    };
    
    /**
     * Hybrid array-list-map container.
     * @arg SIZE - max number of elements
     * @arg HashPolicy - policy contains hash functions h1() and h2()
     *      for double-hashing. Requirements: h2() must return odd values.
     */
    template <
        typename keyT,
        typename objT,
        size_t N,
        class Hash = FNV_1<keyT>
    > class hashlist {
        enum : size_t { SIZE = N };
        using offset_t = int_least32_t;
        
        static_assert((SIZE & (SIZE - 1)) == 0, LOG_HEAD
            "size of the container must be power of 2 (i.e. 2^n): "
            "compile-time tuning will be implemented later");
        
        static_assert(SIZE <= (1UL << (sizeof(offset_t)/2*CHAR_BIT)), LOG_HEAD
            "list size must be less than 2^16, otherwise navigation will be invalid, "
            "please contact to your programmer to fix this");
        
        struct cell_t;
        
        template <typename DefPtr, typename ValPtr>
        class iterator_base : public std::iterator_traits<ValPtr> {
        private:
            using it = std::iterator_traits<ValPtr>;
            friend class hashlist;
            
            /// Pointer to cell
            DefPtr ptr_;
            
            /// Can be constructed only by hashlist
            iterator_base(DefPtr ptr) : ptr_(ptr) {}
            
        public:
            using iterator_category = std::bidirectional_iterator_tag;
            
            /// Other typedefs are inherited from traits: iterator ~ pointer
            
            // ###################### Iterator ###################### //
            
            iterator_base(iterator_base const&) = default;
            
            /// Constructs const_iterator from iterator
            template <
                typename D,
                typename V,
                typename = typename std::enable_if<
                    std::is_convertible<D, DefPtr>::value &&
                    std::is_convertible<V, ValPtr>::value
                >::type
            > iterator_base(iterator_base<D,V> const& other) :
                iterator_base(other.ptr_) {}
            
            iterator_base& operator=(iterator_base const& other) = default;
            
            ~iterator_base() = default;
            
            void swap(iterator_base& other) noexcept {
                std::swap(ptr_, other.ptr_); }
            
            inline typename it::reference operator*() const {
                return ptr_->value(); }
            
            inline typename it::pointer operator->() const {
                return &(this->operator*()); }
            
            iterator_base& operator++() {
                ptr_ += ptr_->next_offset;
                return *this;
            }
            
            // ###################### InputIterator ###################### //
            
            friend const bool operator==(iterator_base const& lh, iterator_base const& rh) {
                return lh.ptr_ == rh.ptr_; }
            
            friend const bool operator!=(iterator_base const& lh, iterator_base const& rh) {
                return !(lh.ptr_ == rh.ptr_); }
            
            iterator_base operator++(int) {
                iterator_base old(*this);
                this->operator++();
                return old;
            }
            
            // ###################### ForwardIterator ###################### //
            
            iterator_base(); // ?
            
            // TODO: see http://en.cppreference.com/w/cpp/concept/ForwardIterator
            
            // ###################### BidirectionalIterator ###################### //
            
            iterator_base& operator--() {
                ptr_ += ptr_->prev_offset;
                return *this;
            }
            
            iterator_base operator--(int) {
                iterator_base old(*this);
                this->operator--();
                return old;
            }
            
            // --- RandomAccessIterator can't be implemented efficiently -- ///
        };
        
    public:
        using key_type          = keyT;
        using mapped_type       = objT;
        using value_type        = std::pair<const keyT, objT>;
        using reference         = value_type&;
        using const_reference   = value_type const&;
        using difference_type   = typename std::pointer_traits<value_type*>::difference_type;
        using size_type         = typename std::make_unsigned<difference_type>::type;
        using hasher            = Hash;
        
        using iterator               = iterator_base<cell_t*,       value_type*>;
        using const_iterator         = iterator_base<cell_t const*, value_type const*>;
        using reverse_iterator       = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        
        /// Default c-tor, leaves storage uninitialized
        hashlist() :
            header_(0x0) {
            cells_[0].next_offset = 0;
            cells_[0].prev_offset = 0;
        }
        
        /// STL-like way: wont compile if value_type isn't copyable
        hashlist(hashlist const& other) : hashlist() {
            for(auto const& p : other)
                push_back(p);
        }
        
        hashlist& operator=(hashlist const& other) = delete;
        
        ~hashlist() {
            clear(); }
        
        
        friend const bool operator==(hashlist const& lh, hashlist const& rh) {
            return lh.size() == rh.size() && std::equal(lh.begin(), lh.end(), rh.begin()); }
        
        friend const bool operator!=(hashlist const& lh, hashlist const& rh) {
            return !(lh == rh); }
        
        /// WARNING: uses binary swap for storage instead of value_type::swap
        friend void swap(hashlist const& lh, hashlist const& rh) {
            using std::swap;
            swap(lh.header_, rh.header_);
            swap(lh.cells_,  rh.cells_);
        }
        
        
        // ###################### Capacity ###################### //
        
        const size_type size() const {
            return header_.count(); }
        
        static constexpr const size_type max_size() {
            return SIZE; }
        
        const bool empty() const {
            return cells_[0].next_offset == 0; }
        
        const float load_factor() const {
            return size() / max_size(); }
        
        
        // ###################### Modifiers ###################### //
        
        void clear() {
            for(auto i = begin(), e = end(); i != e; i = erase(i)); }
        
        /// Provides access to unitialized sentinel's object.
        mapped_type const& get_sentinel() const {
            return cells_[0].value().second; }
        
        mapped_type& get_sentinel() {
            return const_cast<mapped_type&>(static_cast<hashlist const*>(this)->get_sentinel()); }
        
        /**
         * Constructs element in-place.
         * Provides strong exception guarantee.
         * @returns an iterator to the inserted element 
         * TODO: to standard, 3 overloads required under hood
         */
        template <typename... Args>
        iterator emplace_back(key_type const& key, Args&&... args);
        
        iterator push_back(const_reference value) {
            return emplace_back(value.first, value.second); }
        
        const_iterator find(key_type const& key) const {
            return const_iterator(find_cell(key)); }
        
        iterator find(key_type const& key) {
            return iterator(const_cast<cell_t*>(find_cell(key))); }
        
        /**
         * @returns a reference to found or newly inserted (push_back'ed) value
         * TODO: ambiguous meaning for array[] and map[], implement later
         */
        mapped_type& operator[](key_type const& key) = delete;
        
        /// @returns iterator following the removed element
        iterator erase(const_iterator pos) {
            return iterator(remove_cell(const_cast<cell_t*>(pos.ptr_))); }
        
        
        // ###################### Iterators ###################### //
        
        iterator                begin()         { return iterator(&cells_[cells_[0].next_offset]); }
        
        const_iterator          begin()   const { return const_iterator(&cells_[cells_[0].next_offset]); }
        
        const_iterator          cbegin()  const { return begin(); }
        
        
        iterator                end()           { return iterator(cells_.begin()); }
        
        const_iterator          end()     const { return const_iterator(cells_.cbegin()); }
        
        const_iterator          cend()    const { return end(); }
        
        
        reverse_iterator        rbegin()        { return reverse_iterator(end()); }
        
        const_reverse_iterator  rbegin()  const { return const_reverse_iterator(cend()); }
        
        reverse_iterator        rend()          { return reverse_iterator(begin()); }
        
        const_reverse_iterator  rend()    const { return const_reverse_iterator(cbegin()); }
        
        
        // ###################### Access ###################### //
        
        const_reference         front()   const { return *cbegin(); }
        
        reference               front()         { return *begin(); }
        
        const_reference         back()    const { return *(--cend()); }
        
        reference               back()          { return *(--end()); }
        
        /// @returns offset of iterator's element inside underlying array
        const size_t offset_of_element(const_iterator const& citer) const {
            return citer.ptr_ - &(cells_[0]); }
        
        /// @returns iterator by elements offset
        iterator element_by_offset(size_t idx) {
            return iterator(&cells_[idx]); }
        
    private:
        struct cell_t {
            offset_t next_offset;
            offset_t prev_offset;
            typename std::aligned_storage<
                sizeof(value_type),
                alignof(value_type)
            >::type value_;
            
            value_type const& value() const {
                return *reinterpret_cast<const value_type*>(&value_); }
            
            value_type& value() {
                return const_cast<value_type&>(static_cast<cell_t const*>(this)->value()); }
        };
        
        std::bitset<SIZE> header_;
        std::array<cell_t, SIZE + 1> cells_;
        
        /// General implementation, requires only key moving, TODO
        template <typename K, typename... Args>
        void emplace_back_impl(K&& key, Args&&... args);
        
        /// @returns pointer to found cell, &sentinel (==end()) if doesn't exists
        cell_t const* find_cell(keyT const& key) const;
        
        /// Removes cell, @returns pointer to cell following the removed one
        cell_t* remove_cell(cell_t* cell);
    };
    
} // hl
} // ax

#include <ax.hashlist_impl.hpp>

#undef LOG_HEAD
