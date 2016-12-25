#include <ax.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <list>
#include <map>
#include <memory>
#include <fstream>

#include <ax.hashlist.hpp>

using namespace ax;

struct counter {
    static size_t ctor_counter;
    static size_t dtor_counter;
    
    counter(int)  { ++ctor_counter; }
    ~counter()    { ++dtor_counter; }
};
size_t counter::ctor_counter = 0;
size_t counter::dtor_counter = 0;

void haslist_test() {
    const size_t N = 2048;
    using hl_t = hl::hashlist<int, int, N>;
    hl_t l;
    
    LIGHT_TEST(std::is_standard_layout<decltype(l)>::value);
    
    LIGHT_TEST(l.max_size() == N);
    
    size_t amount = l.max_size();
    for(int i = 0; i < amount; ++i) {
        l.emplace_back(i, i*i);
        LIGHT_TEST(l.back().first == i);
        LIGHT_TEST(l.back().second == i*i);
        LIGHT_TEST(l.size() == static_cast<size_t>(i + 1));
    }
    
    LIGHT_TEST(l.load_factor() == 1.0f);
    
    bool overflow = false;
    try {
        l.emplace_back(-1, -1);
    } catch(std::bad_alloc&) {
        overflow = true;
    }
    
    LIGHT_TEST(overflow == true);
    
    auto r = l;
    
    LIGHT_TEST(l == r);
    
    l.clear();
    
    LIGHT_TEST(l != r);
    
    LIGHT_TEST(l.size() == 0);
    
    LIGHT_TEST(l.load_factor() == 0.0f);
    
    auto& hlist = l;
    std::list<hl_t::value_type> slist;
    
    for(size_t i = 0; i < 32_KIB; ++i) {
        int key = std::rand();
        int val = std::rand();
        
        LIGHT_TEST(hlist.size() == slist.size());
        
        if(slist.size() < amount) { // not full
            auto hfound = hlist.find(key);
            auto sfound = std::find_if(
                slist.begin(), slist.end(),
                [&key](hl_t::value_type const& p){
                    return p.first == key;
                });
            
            if(hfound == hlist.end()) { // no such key
                LIGHT_TEST(sfound == slist.end());
                hlist.emplace_back(key, val);
                slist.emplace_back(key, val);
            } else { // there is key, remove
                LIGHT_TEST(sfound != slist.end());
                LIGHT_TEST(*sfound == *hfound);
                hlist.erase(hfound);
                slist.erase(sfound);
            }
        
        } else { // full
            size_t idx = key % amount;
            auto hiter = hlist.begin();
            auto siter = slist.begin();
            
            for(size_t j = 0; j < idx; ++j) {
                (j % 2 == 0) ? ++hiter : hiter++;
                (j % 2 == 0) ? ++siter : siter++;
            }
            
            hlist.erase(hiter);
            slist.erase(siter);
            
            hlist.erase(hlist.begin());
            slist.erase(slist.begin());
            
            hlist.erase(--hlist.end());
            slist.erase(--slist.end());
            
            hlist.erase(hlist.begin());
            slist.erase(slist.begin());
            
            hlist.erase(--hlist.end());
            slist.erase(--slist.end());
        }
        
        if(i % 1_KIB == 0) {
            decltype(slist) temp(hlist.begin(), hlist.end());
            bool equal = std::equal(slist.begin(), slist.end(), hlist.begin());
            LIGHT_TEST(equal);
        }
    }
    
    {
        // Special stored type
        using hlp_t = hl::hashlist<int, std::unique_ptr<int>, N>;
        
        hlp_t lp;
        lp.emplace_back(0, nullptr);
        // lp.push_back(std::make_pair(0, nullptr)); // error
        // auto rp = lp; // error
    }
    
    {
        // Construction-destruction
        using hlc_t = hl::hashlist<int, counter, N>;
        hlc_t cl;
        
        for(size_t i = 0; i < cl.max_size(); ++i)
            cl.emplace_back(i, i);
        
        LIGHT_TEST(counter::ctor_counter == cl.max_size());
        
        cl.clear();
        
        LIGHT_TEST(counter::dtor_counter == cl.max_size());
    }
    
    {
        // Multimap
        using hlm_t = hl::hashlist<size_t, size_t, N>;
        
        for(const size_t div : {
            size_t{1},
            size_t{16}, 
            size_t{256},
            size_t{N}
        }) {
            hlm_t ml;
            
            for(size_t i = 0; i < N; ++i)
                ml.emplace_back(i/div, i);
            
            LIGHT_TEST(ml.size() == N);
            
            for(size_t i = 0; i < N/div; ++i) {
                auto range = std::equal_range(
                    ml.cbegin(),
                    ml.cend(),
                    hlm_t::value_type{i, 0},
                    [](hlm_t::value_type const& lh, hlm_t::value_type const& rh) {
                        return lh.first < rh.first;
                    });
                
                size_t length = std::distance(range.first, range.second);
                LIGHT_TEST(length == div);
                
                size_t j = 0;
                for(auto it = range.first; it != range.second; ++it, ++j)
                    LIGHT_TEST(it->second == i*div + j);
            }
        }
    }
}

void perf() {
    struct dummy_t {
        std::array<char, 32> data;
        dummy_t(char c) :
            data{42} {
            volatile char cv = c;
            data[0] = cv;
            data[data.size()-1] = cv;
        };
    };
    
    using hl_t = hl::hashlist<size_t, dummy_t, 1_KIB>;
    
    hl_t l;
    
    const size_t size = hl_t::max_size();
    const size_t runs = 256;
    
    std::array<std::array<size_t, size>, runs> allocations;
    std::array<size_t, runs> rows;
    
    for(size_t r = 0; r < runs; ++r) {
        l.clear();
        auto& run = allocations[r];
        
        auto row = rdtsc();
        for(size_t i = 0; i < size; ++i) {
            auto t = rdtsc();
            volatile size_t vi = i;
            i = vi;
            
            l.emplace_back(i, char(i));
            auto back = l.back();
            
            volatile auto b1 = back.first;
            volatile auto b2 = back.second.data[0];
            volatile auto b3 = back.second.data[l.back().second.data.size() - 1];
            b1 = b2 = b3 = b1;
            
            run[i] = rdtsc() - t;
        }
        rows[r] = rdtsc() - row;
    }
    
    std::ofstream out;
    out.open("perf.txt");
    out << "emplace";
    for(size_t s = 0; s < size; ++s)
        out << "\t" << (s + 1);
    out << "\n";
    for(size_t r = 1; r < runs; ++r) {
        out << r;
        for(auto push : allocations[r])
            out << "\t" << push;
        out << "\n";
    }
    out << "\n\n";
    
    size_t findLF = 10;
    
    for(size_t r = 1; r <= findLF; ++r) {
        l.clear();
        
        auto& run = allocations[r];
        size_t sizeLF = size*r/findLF;
        
        for(size_t i = 0; i < sizeLF; ++i)
            l.emplace_back(i, char(i));
        
        for(size_t i = 0; i < size; ++i) {
            auto key = std::rand() % (sizeLF);
            
            auto t = rdtsc();
            auto found = l.find(key);
            volatile auto f1 = found->first;
            volatile auto f2 = found->second.data[0];
            volatile auto f3 = found->second.data[l.back().second.data.size() - 1];
            f1 = f2 = f3 = f1;
            run[i] = rdtsc() - t;
        }
    }
    
    for(size_t s = 1; s <= findLF; ++s)
        out << "\t" << s*10 << "%";
    out << "\n";
    for(size_t i = 0; i < size; ++i) {
        for(size_t r = 1; r <= findLF; ++r)
            out << "\t" << allocations[r][i];
        out << "\n";
    }
    
    out << "\n\n";
    
    out.close();
}

int main() {
    haslist_test();
    perf();
}