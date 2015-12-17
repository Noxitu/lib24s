FLAGS=-std=c++11 -fdiagnostics-color=always -Wall -pedantic

24: src/main.cpp src/error.cpp src/*.hpp
	g++ $(FLAGS) -o 24 src/main.cpp src/error.cpp -ldl
	
all: 24 baj.so
  
test: all
	./24

baj.so: baj/*.cpp baj/*.hpp src/*.hpp src/error.cpp
	g++ $(FLAGS) -shared -fpic baj/*.cpp src/error.cpp -o baj.so -I"src/"
  
clean:
	rm ./24 ./baj.so