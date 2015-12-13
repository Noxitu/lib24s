FLAGS=-std=c++11 -fdiagnostics-color=always

all: src/main.cpp src/*.hpp
	g++ $(FLAGS) -o 24 src/main.cpp -ldl
	
test: all
	./24

baj.so: baj/baj.cpp src/*.hpp src/error.cpp
	g++ $(FLAGS) -shared -fpic baj/baj.cpp src/error.cpp -o baj.so -I"src/"