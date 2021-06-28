#ifndef MULTH_LISTWORKER_HG
#define MULTH_LISTWORKER_HG

#include <vector>
#include <iostream>
#include <functional>

// multithreading
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

template <typename O>
struct Listworker_ini {
    std::function<void(O*)> process_element;
    std::function<void(std::vector<O*>*)> cycle_end;
    uint64_t thread_count = 2;
    std::chrono::duration<int64_t, std::milli> cycletime = std::chrono::milliseconds(1000);
    uint64_t del_it_pos = 0;
};

template <typename O>
// class O must have:
//      std::atomic<uint64_t> multh_del_it[1] = {0xFFFFFFFFFFFFFFFF};
//      std::atomic<bool> multh_added[1] = false;

class Listworker {
  public:
    bool is_ini = false;
    uint64_t del_it_pos = 0;
    
    std::vector<O*> main_list;
    
    std::vector<O*> add_list;
    std::vector<O*> del_list;
    std::mutex addel_mtx;
    
    uint64_t thread_count = 0;
    std::vector<std::thread> threads;
    
    bool w = false;
    std::atomic<uint64_t> main_list_it;
    std::mutex it_reset_mtx;
    
    // condition_variable that is called everytime when a new cycle begins
    std::mutex next_cycle_mtx;
    std::condition_variable next_cycle_cv;
    
    std::chrono::duration<int64_t, std::milli> cycletime;
    std::chrono::steady_clock::time_point cycle_starts;
    std::chrono::steady_clock::time_point now;
    
    std::function<void(O*)> process_element;
    std::function<void(std::vector<O*>*)> cycle_end;
    
    ////////////////////////////////////////////////////
    // Constructor / Destructor
    ////////////////////////////////////////////////////
    
    Listworker () {}
    
    Listworker (Listworker_ini<O> ini) {
        if (!ini.process_element) {
            // throw
            std::cout << "Listworker initialization fails due nullptr as element-function\n";
            return;
        }
        
        this->process_element = ini.process_element;
        this->cycle_end = ini.cycle_end;
        this->cycletime = ini.cycletime;
        this->thread_count = ini.thread_count;
        this->del_it_pos = ini.del_it_pos;
        
        this->is_ini = true;
    }
    
    ~Listworker () {
        if (this->w) {
            this->w = false;
            
            this->halt();
            
            this->next_cycle_cv.notify_all();
            
            for (auto thread_it = this->threads.begin(); thread_it != this->threads.end(); ++thread_it) {
                thread_it->join();
            }
        }
    }
    
    ////////////////////////////////////////////////////
    // inlie methodes to control from extern
    ////////////////////////////////////////////////////
    
    inline bool ini(Listworker_ini<O> ini) {
        // worker allready running
        if (this->w) {
            return false;
        }
        
         if (!ini.process_element) {
            // throw
            std::cout << "Listworker initialization fails due nullptr as element-function\n";
            return false;
        }
        
        this->process_element = ini.process_element;
        this->cycle_end = ini.cycle_end;
        this->cycletime = ini.cycletime;
        this->thread_count = ini.thread_count;
        this->del_it_pos = ini.del_it_pos;
        
        this->is_ini = true;
        return true;
    }
    
    // start with an numer of threads
    inline void start() {
        // not initialized
        if (!this->is_ini) {
            return;
        }
        
        if (this->w) {
            return;
        }
        
        this->w = true;
        for (uint64_t i = 0; i < this->thread_count; ++i) {
            this->threads.push_back(std::thread(&Listworker<O>::main_loop, this));
        }
    }
    
    // pause the workers
    inline void halt() {
        this->it_reset_mtx.lock();
    }
    
    //!!! if not properly locked cause undefiened behavour!!!
    // continue after halt()
    inline void cont() {
        this->it_reset_mtx.unlock();
        this->next_cycle_cv.notify_all();
    }
    
    // queue Object for adding
    inline void add(O* ptr) {
        if (!ptr->multh_added[this->del_it_pos].exchange(true)) { // modify to_wake
            std::lock_guard<std::mutex> lck(this->addel_mtx);
            this->add_list.push_back(ptr);
        }
    }
    
    // queue Object for deleting
    inline void del(O* ptr) {
        if (ptr->multh_del_it[this->del_it_pos] != 0xFFFFFFFFFFFFFFFF && ptr->multh_added[this->del_it_pos].exchange(false)) {
            std::lock_guard<std::mutex> lck(this->addel_mtx);
            this->del_list.push_back(ptr);
        }
    }
    
    // test if an Object is in main_list or in add_list
    inline bool is_added(O* ptr) const {
        return ptr->multh_added[this->del_it_pos].load();
    }
    
    ////////////////////////////////////////////////////
    // intern methodes
    ////////////////////////////////////////////////////
    
    void main_loop() {
        uint64_t loc_it;
        
        while (this->w) {
            loc_it = this->main_list_it++; // get new loc_its
            if (loc_it >= this->main_list.size()) {
                //loc_it is invalid:
                
                // one thread resets the map_it when a new tick must be done
                if (this->it_reset_mtx.try_lock()) {
                    std::unique_lock<std::mutex> it_reset_lck (this->it_reset_mtx, std::adopt_lock); // package the mutex in a lock_gaurd if it is locked
                    
                    // only read (and lock) add_list / del_list if needed
                    if (this->add_list.size() + this->del_list.size() > 0) {
                        addel();
                    }
                    
                    // get the cycle_end function executed (only if set)
                    if (this->cycle_end) {
                        this->cycle_end(&this->main_list);
                    }
                    
                    // timehandeling and sleep
                    this->now = std::chrono::steady_clock::now();
                    this->cycle_starts += this->cycletime;
                    
                    if (this->now < this->cycle_starts) {
                        std::this_thread::sleep_until(this->cycle_starts);
                    } else {
                        //? add functionality to detect too slow systems
                        
                        this->cycle_starts = std::chrono::steady_clock::now();
                    }
                    
                    // cleanup and reset
                    this->main_list_it = 0;
                    it_reset_lck.unlock();
                    
                    this->next_cycle_cv.notify_all();
                } else {
                    // wait until the next tick begins
                    std::unique_lock<std::mutex> lck(this->next_cycle_mtx);
                    this->next_cycle_cv.wait_for(lck, this->cycletime * 16); // wait_for to prevent deadlocks
                }
            } else {
                // loc_it is valid:
//                 std::cout << "get to work\n";
                process_element(main_list[loc_it]);
            }
        }
    }
    
    //# mutithread addel
    inline void addel() {
        std::lock_guard<std::mutex> lck(this->addel_mtx); // lock for modification
        
        int64_t advance = this->add_list.size() - this->del_list.size(); // get how much the main_list size must tweaked
        int64_t replace = (this->add_list.size() < this->del_list.size()) ? this->add_list.size() : this->del_list.size(); // get the number of elements wich can easily replaced
        uint64_t i = 0;
        
        uint64_t tmp;
        
        // work up the smaller list with just replaces
        for (; i < replace; ++i) {
            // switch multh_del_it of the elements that shold be replaced
            tmp = this->del_list[i]->multh_del_it[del_it_pos].load();
            this->add_list[i]->multh_del_it[del_it_pos] = tmp;
            this->del_list[i]->multh_del_it[del_it_pos] = 0xFFFFFFFFFFFFFFFF;
            // replace the element
            this->main_list[tmp] = this->add_list[i];
        }
        
        if (advance == 0) {
            // skip directly to cleanup
        } else if (advance > 0) {
            // main_list must be expanded:
            i = this->main_list.size();
            this->main_list.insert(this->main_list.end(), this->add_list.begin(),  this->add_list.end()); // expand with an insert
            
            tmp = this->main_list.size();
            for (; i < tmp; ++i) { // write multh_del_it in all new inserted elements
                this->main_list[i]->multh_del_it[del_it_pos] = i;
            }
        } else {
            // main_list must shrink:
            //# shrink with resize?
            this->main_list.erase(this->main_list.end() + advance, this->main_list.end()); // shrink with an erase
            
            replace -= advance;
            for (; i < replace; ++i) {
                this->del_list[i]->multh_del_it[del_it_pos] = 0xFFFFFFFFFFFFFFFF;
            }
        }
        // cleanup
        this->del_list.clear();
        this->add_list.clear();
    }
};


#endif // HEADDER_GUARD
