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
        size_t size;
        size_t capacity;
        Element data[];
        
        Bucket () {
            this->data = nullptr;
            this->capacity = 0;
            this->size = 0;
        }
        
        Bucket (const size_t cap) {
            data = new Element[cap];
            this->capacity = cap;
            this->size = 0;
        }
        
        bool add (const Element& new_el) {
            {
                Element* it = this->data;
                const Element* end = this->data + size;
                
                while (it != end) { // check that no elements with the same key exists
                    if (it->first == new_el.first)
                        return false;
                    it++;
                }
            }
            
            this->size++;
            if (this->size == this->capacity) {
                this->capacity += 2;
                
                const Element[] old_data = this->data;
                this->data = new Element[this->capacity];
                
                if (old_data) { // if old_data was actually used
                    std::memcpy(this->data, old_data, size * size_of(Key_Type));
                    delete[](old_data, size);
                }
            }
            
            this->data[size] = new_el;
            return true;
        }
        
        Data_Type* del (const Key_Type key) {
            Element* it = this->data;
            const Element* end = this->data + size;
            
            while (it->first != key) {
                if (it == end)
                    return nullptr;
                it++;
            }
            
            const Element* last = end - 1;
            if (it != last)
                *it = *last;
            
            this->size--;
            
            return last->second;
        }
        
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
