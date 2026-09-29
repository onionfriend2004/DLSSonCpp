CXX      = g++
CXXFLAGS = -O2 -Wall -Wextra

run: main.o io.o codec.o
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -f *.o run
