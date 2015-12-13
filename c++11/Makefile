FLAGS=-std=c++11 -fdiagnostics-color=always

24: src/main.cpp src/error.cpp src/*.hpp
	g++ $(FLAGS) -o 24 src/main.cpp src/error.cpp -ldl
	
all: 24 baj.so
  
test: all
	./24

baj.so: baj/baj.cpp src/*.hpp src/error.cpp
	g++ $(FLAGS) -shared -fpic baj/baj.cpp src/error.cpp -o baj.so -I"src/"
  
clean:
	rm ./24 ./baj.so