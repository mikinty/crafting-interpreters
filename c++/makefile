FMT=fmt/include
INC=$(FMT)
INC_PARAMS=$(foreach d, $(INC), -I$d)

build: main.cpp
	$(MAKE) clean
	g++ -g -Wall -std=c++2a $(INC_PARAMS) -DFMT_HEADER_ONLY main.cpp chunk.cpp vm.cpp compiler.cpp scanner.cpp -o main

clean: 
	del *.exe