
#include "multh_listworker.hpp"
#include <iostream>
#include <thread>
#include <chrono>

// global print mutex, so only one thread can print at a time
static std::mutex print_mtx;

class TestClass {
  public:
    // boolean flag, if this package should be heavy
    bool unique = false;
    
    // used by work_one
    uint64_t counter_start = 0;
    uint64_t last_counter = 0;
    uint64_t nr_counter = 0;
    
    // used by work_two / work_three
    uint64_t priv_cycle = 0;
    
    // the public members required by multh::Listworker
    std::atomic<uint64_t> multh_del_it[2] = {0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF};
    std::atomic<bool> multh_added[2] = {false, false};
};

// just increment nr_counter
void work_one (TestClass* subject, uint64_t cycle) {
    
    if (subject->nr_counter == 0) {
        subject->counter_start = cycle;
    }
    
    subject->nr_counter++;
    subject->last_counter = cycle;
}

// must be exclusive with work_three
void work_two (TestClass* subject, uint64_t cycle) {
    
    // if this is the first cycle for this 
    if (subject->priv_cycle == 0) {
        subject->priv_cycle = cycle;
        return;
    }
    
    // with up to no load, each cycle should come to every subject
    if (++subject->priv_cycle != cycle) {
        std::lock_guard<std::mutex> print_lck(print_mtx);
        std::cerr << "Error in work_two in line " << __LINE__ << " of " << __FILE__ << "\n    cycle nr. " << subject->priv_cycle << " expected, but got " << cycle << ".\n";
        exit(1);
    }
}

// must be exclusive with work_two
void work_three (TestClass* subject, uint64_t cycle) {
    
    // if this is the first cycle for this 
    if (subject->priv_cycle == 0) {
        subject->priv_cycle = cycle;
        return;
    }
    
    // cycle skipping is okay under heavy load, but should keep in order
    if (++subject->priv_cycle != cycle) {
        if (subject->priv_cycle < cycle) {
            std::lock_guard<std::mutex> print_lck(print_mtx);
            std::cout << "At unique == " << subject->unique << ": skipped " << cycle - subject->priv_cycle << " cycles.\n";
        } else {
            std::lock_guard<std::mutex> print_lck(print_mtx);
            std::cerr << "Error in work_three in line " << __LINE__ << " of " << __FILE__ << "\n    got cycle nr. " << cycle << " but was already in " << subject->priv_cycle - 1 << ".\n";
            exit(1);
        }
    }
    
    
    // test for exceptions if work is to heavy
    if (subject->unique) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main () {
    multh::Listworker_ini<TestClass> lw1_ini;
    lw1_ini.process_element = work_one;
    lw1_ini.thread_count = 3;
    lw1_ini.cycle_time = std::chrono::milliseconds(10);
    lw1_ini.del_it_pos = 0;
    
    multh::Listworker_ini<TestClass> lw2_ini;
    lw2_ini.process_element = work_two;
    lw2_ini.cycle_end = [](std::vector<TestClass*>* list)->void{
        std::lock_guard<std::mutex> print_lck(print_mtx);
        std::cout << "through one cycle\n";
    };
    lw2_ini.thread_count = 7;
    lw2_ini.cycle_time = std::chrono::milliseconds(50);
    lw2_ini.del_it_pos = 1;
    
    multh::Listworker<TestClass> lw1;
    multh::Listworker<TestClass> lw2;
    
    lw1.ini(lw1_ini);
    lw2.ini(lw2_ini);
    
    TestClass tests[100]; // create a few tests
    
    for (uint64_t i = 0; i < 100; ++i) {
        lw1.add(&tests[i]);
        lw2.add(&tests[i]);
    }
    
    lw1.start();
    lw2.start();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    return 0;
}

