#include <iostream>
#include <vector>
#include <atomic>
#include <chrono>

#include "multh_listworker.hpp"


class Testclass {
public:
    uint64_t number;
    std::vector<int> data;
    
    Testclass () {}
    
    Testclass (int number) {
        this->number = number;
    }
    
    ~Testclass () {}
    
    // needed for multh_listworker
    std::atomic<uint64_t> multh_del_it[4] = {0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF};
    std::atomic<bool> multh_added[4] = {false, false, false, false};
};

void work(Testclass* a) {
    if (a->data.size() < 2) {
        a->data.push_back(1);
    } else {
        a->data[0] += a->data[1];
    }
}



int main() {
    
    int iterator = 0;
    
    Listworker_ini<Testclass> tmp;
    tmp.process_element = &work;
    tmp.thread_count = 7;
    tmp.del_it_pos = 3;
    tmp.cycletime = std::chrono::milliseconds(20);
    
    tmp.cycle_end = [&iterator](std::vector<Testclass*>* list)->void {
        if (list->size() > iterator) {
            if (list->at(iterator)->data.size() > 1) {
                list->at(iterator)->data[1]++;
            }
        } else {
            iterator = 0;
        }
    };
    
    Listworker<Testclass> a(tmp);
    
    Testclass ts[100000]{5, 6, 7, 8, 9, 10};
    a.add(&ts[0]);
    a.add(&ts[1]);
    a.add(&ts[2]);
    a.add(&ts[3]);
    a.add(&ts[4]);
    
    a.start();
    
    for (uint64_t i = 0; i < 100000; ++i) {
        a.add(&ts[i]);
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    for (uint64_t i = 0; i < 100000; ++i) {
        a.del(&ts[i]);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    return 0;
}
