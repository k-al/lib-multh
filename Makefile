CFLAGS = -std=c++17 -O3 -pthread

ALL_TESTS = tests/Listworker_t01.app tests/Listworker_t02.app

clean:
	rm ./tests/*.app

test: $(ALL_TESTS)
	./tests/Listworker_t01.app
	./tests/Listworker_t02.app

tests/Listworker_t01.app: tests/listworker_t01.cpp lib/multh_listworker.hpp
	g++ $(CFLAGS) -I./lib/ -o tests/Listworker_t01.app tests/listworker_t01.cpp

tests/Listworker_t02.app: tests/listworker_t02.cpp lib/multh_listworker.hpp
	g++ $(CFLAGS) -I./lib/ -o tests/Listworker_t02.app tests/listworker_t02.cpp
