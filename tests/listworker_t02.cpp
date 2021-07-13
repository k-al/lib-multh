
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
    
    TestClass () {};
    
    TestClass (bool unique) {
        this->unique = unique;
    }
    
    ~TestClass () {
        if (this->nr_counter != 0) {
            if (this->last_counter - this->counter_start + 1 != this->nr_counter) {
                std::lock_guard<std::mutex> print_lck(print_mtx);
                std::cerr << "Error in work_one in " << __FILE__ << "\n    got " << this->nr_counter << " from cycle " << this->counter_start << " to cycle " << this->last_counter << ".\n";
                exit(1);
            }
        }
    }
    
    
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
        exit(2);
    }
}

// must be exclusive with work_two
void work_three (TestClass* subject, uint64_t cycle) {
    
    // if this is the first cycle for this 
    if (subject->priv_cycle == 0) {
        subject->priv_cycle = cycle;
        
        if (subject->unique) {
            std::lock_guard<std::mutex> print_lck(print_mtx);
            std::cout << "get the specials\n";
        }
        
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
            exit(3);
        }
    }
    
    
    // test for exceptions if work is to heavy
    if (subject->unique) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

int main () {
    { // first part of the test (work_one and work_two)
        TestClass tests[100]; // create a few tests
        
        multh::Listworker_ini<TestClass> lw1_ini;
        lw1_ini.process_element = work_one;
        lw1_ini.thread_count = 3;
        lw1_ini.cycle_time = std::chrono::milliseconds(10);
        lw1_ini.del_it_pos = 0;
        
        multh::Listworker_ini<TestClass> lw2_ini;
        lw2_ini.process_element = work_two;
        lw2_ini.cycle_end = [](std::vector<TestClass*>* list)->void{
            std::lock_guard<std::mutex> print_lck(print_mtx);
            std::cout << "through one cycle in work_two\n";
        };
        lw2_ini.thread_count = 7;
        lw2_ini.cycle_time = std::chrono::milliseconds(50);
        lw2_ini.del_it_pos = 1;
        
        multh::Listworker<TestClass> lw1;
        multh::Listworker<TestClass> lw2;
        
        lw1.ini(lw1_ini);
        lw2.ini(lw2_ini);
        
        for (uint64_t i = 0; i < 100; ++i) {
            lw1.add(&tests[i]);
            lw2.add(&tests[i]);
        }
        
        lw1.start();
        lw2.start();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    { // second part of the test (work_three)
        // create a few tests
        TestClass tests[10000]{true, true}; // make the first two to specials, that have "heavy load"
        
        multh::Listworker_ini<TestClass> lw3_ini;
        lw3_ini.process_element = work_three;
        lw3_ini.cycle_end = [](std::vector<TestClass*>* list)->void{
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            std::lock_guard<std::mutex> print_lck(print_mtx);
            std::cout << "through one cycle in work_three\n";
        };
        lw3_ini.thread_count = 8;
        lw3_ini.cycle_time = std::chrono::milliseconds(100);
        lw3_ini.del_it_pos = 0;
        
        multh::Listworker<TestClass> lw3;
        lw3.ini(lw3_ini);
        
        lw3.start();
        
        // leave out the special ones
        for (uint64_t i = 3; i < 10000; ++i) {
            lw3.add(&tests[i]);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        // add the special ones
        lw3.add(&tests[0]);
        lw3.add(&tests[1]);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    
    return 0;
}

