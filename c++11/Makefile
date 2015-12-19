FLAGS=-std=c++11 -fdiagnostics-color=always -Wall -pedantic -g

24: Makefile src/main.cpp src/error.cpp src/*.hpp
	g++ $(FLAGS) -o 24 src/main.cpp src/error.cpp -ldl
	
all: 24 baj.so oix.so
  
test: 24
	./24

baj.so: Makefile baj/*.cpp baj/*.hpp src/*.hpp src/error.cpp
	g++ $(FLAGS) -shared -fpic baj/*.cpp src/error.cpp -o baj.so -I"src/"
  
oix.so: Makefile oix/*.cpp oix/*.hpp src/*.hpp src/error.cpp
	g++ $(FLAGS) -shared -fpic oix/*.cpp src/error.cpp -o oix.so -I"src/"
  
clean:
	rm ./24 ./baj.so ./oix.so