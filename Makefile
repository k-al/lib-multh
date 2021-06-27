CFLAGS = -std=c++17 -O3 -pthread

ALL_TESTS = tests/Listworker_t01.app

test: $(ALL_TESTS)
	./tests/Listworker_t01.app

tests/Listworker_t01.app: tests/listworker_t01.cpp lib/multh_listworker.hpp
	g++ $(CFLAGS) -I./lib/ -o tests/Listworker_t01.app tests/listworker_t01.cpp