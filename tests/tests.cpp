#include <ax.hpp>

#include <cmath>
#include <list>

#include <ax.hashlist.hpp>

using namespace ax;

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
    
    LIGHT_TEST(l.size() == 0);
    
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
            
            //stdprintf("Removed {%%, %%}", siter->first, siter->second);
        }
        
        if(i % 1_KIB == 0) {
            decltype(slist) temp(hlist.begin(), hlist.end());
            bool equal = std::equal(slist.begin(), slist.end(), hlist.begin());
            LIGHT_TEST(equal);
        }
    }
}

int main() {
    haslist_test();
}