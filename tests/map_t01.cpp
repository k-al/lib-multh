
#include "multh_map.hpp"

#include <iostream>


int main () {
    multh::Map<long, std::string> test_map;
    
    test_map.insert(5, new std::string("hello"));
    
    std::cout << "after inserting\n";
    
    std::cout << "get the element: " << *(test_map.get(5)) << std::endl;
    
    std::cout << "delete sucessfull: " << test_map.erease(5) << std::endl;
}
