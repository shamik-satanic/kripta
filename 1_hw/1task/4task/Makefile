CXX      := g++
CXXFLAGS := -std=c++14 -Wall -Wextra -O2 -pthread
LDFLAGS  := -pthread -lrt

.PHONY: all clean demo test

all: rc4_server rc4_client gen_testfiles

rc4_server: server.cpp rc4.hpp protocol.hpp
	$(CXX) $(CXXFLAGS) -o $@ server.cpp $(LDFLAGS)

rc4_client: client.cpp protocol.hpp rc4.hpp
	$(CXX) $(CXXFLAGS) -o $@ client.cpp $(LDFLAGS)

gen_testfiles: gen_testfiles.cpp
	$(CXX) -std=c++14 -O2 -o $@ $< -lm

rc4_tests: rc4_tests.cpp rc4.hpp
	$(CXX) $(CXXFLAGS) -o $@ rc4_tests.cpp

test: rc4_tests
	./rc4_tests

demo: all
	bash demo.sh

clean:
	rm -f rc4_server rc4_client gen_testfiles rc4_tests
	./rc4_server --clean 2>/dev/null || true