#ifndef MULTH_MAP_HG
#define MULTH_MAP_HG

#include <vector>
#include <utility>


// multithreading
#include <mutex>
#include <shared_mutex>
#include <atomic>

// for debugging
// #include <iostream>

namespace multh {
    
    template<typename Key_Type, typename Data_Type>
    
    class Bucket {
        using Element = std::pair<Key_Type, Data_Type*>;
    public:
        // number of valid elements in the list
        size_t size;
        
        // number of elements this bucket can handle without realocation
        size_t capacity;
        
        // raw c-style array for the data
        Element data[];
        
        // initialising without array allocation
        Bucket () {
            this->data = nullptr;
            this->capacity = 0;
            this->size = 0;
        }
        
        // initialising with predefined capacity
        Bucket (const size_t cap) {
            data = new Element[cap];
            this->capacity = cap;
            this->size = 0;
        }
        
        // add a new element to the bucket (with deduplicating)
        bool add (const Element& new_el) {
            
            { // deduplicating work
                Element* it = this->data;
                const Element* end = this->data + this->size;
                
                while (it != end) { // check that no elements with the same key exists
                    if (it->first == new_el.first)
                        return false;
                    it++;
                }
            } // dedup-end
            
            // increasing container size
            this->size++;
            
            // check if reallocation of the memory must happen
            if (this->size == this->capacity) {
                // increase the capacity linear
                this->capacity += 2;
                
                const Element[] old_data = this->data;
                this->data = new Element[this->capacity];
                
                if (old_data) { // if old_data was actually used
                    std::memcpy(this->data, old_data, this->size * size_of(Element));
                    delete[](old_data, this->size);
                }
            }
            
            // put the element in the last position
            this->data[this->size - 1] = new_el;
            return true;
        }
        
        // remove element associated with the Key from the bucket
        //     and return the pointer to the removed data (nullptr, if key was not found) for further resource management
        Data_Type* del (const Key_Type key) {
            
            // iterate through the list to find 'key'
            Element* it = this->data;
            const Element* end = this->data + size;
            
            while (it->first != key) {
                if (it == end)
                    return nullptr;
                it++;
            }
            
            // save the data pointer before overwriting the element
            const Data_Type* res = it->second;
            
            // switch the found element to the end
            const Element* last = end - 1;
            if (it != last)
                *it = *last;
            
            // tell the bucket to not access the last element anymore
            this->size--;
            
            return res;
        }
        
        // get the data pointer to matching key (nullptr if key is not in bucket)
        Data_Type* get (const Key_Type key) const {
            Element* it = this->data;
            const Element* end = this->data + size;
            
            while (it->first != key) {
                if (it == end)
                    return nullptr;
                it++;
            }
            return it->second;
        }
    }
    
    
    
    template<typename Key_Type, typename Data_Type, typename Hash_Function = std::hash<Key_Type>>
    
    class Map {
    public:
        std::shared_mutex rehash_mtx;
        std::vector<std::mutex> bucket_mutex;
        std::unordered_map<Key_Type, Data_Type, Hash_Function> data;
        
        
        // get the data belonged to *key*
        Data_Type* get (Key_Type key);
        
        // get the data belonged to *key* and keep the bucket locked with the *guard*
        Data_Type* get (Key_Type key, std::lock_guard& guard);
        
        // returns true if the pair [key, data] was succesfully added
        bool insert (Key_Type key, Data_Type data);
        
        
    }
    
}

#endif
