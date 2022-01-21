CFLAGS = -std=c++17 -O3 -pthread -Wall

clean:
	rm ./tests/*.app

test-map: tests/Map_t01.app
	#
	#
	#
	# Test:  ---  Map  ---
	#
	./tests/Map_t01.app

test-listworker: tests/Listworker_t01.app tests/Listworker_t02.app
	#
	#
	#
	# Test:  ---  Listworker  ---
	#
	./tests/Listworker_t01.app
	#
	./tests/Listworker_t02.app

test: test-listworker test-map

tests/Listworker_t01.app: tests/listworker_t01.cpp lib/multh_listworker.hpp
	echo "Test:  ---  Listworker_t01  ---"
	g++ $(CFLAGS) -I./lib/ -o tests/Listworker_t01.app tests/listworker_t01.cpp

tests/Listworker_t02.app: tests/listworker_t02.cpp lib/multh_listworker.hpp
	g++ $(CFLAGS) -I./lib/ -o tests/Listworker_t02.app tests/listworker_t02.cpp

tests/Map_t01.app: tests/map_t01.cpp lib/multh_map.hpp
	g++ $(CFLAGS) -I./lib/ -o tests/Map_t01.app tests/map_t01.cpp
