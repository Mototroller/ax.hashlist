#include <ax.hpp>

#include <cmath>
#include <list>
#include <memory>

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
    
    LIGHT_TEST(l.max_size() == N - 1);
    
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
}

int main() {
    haslist_test();
}