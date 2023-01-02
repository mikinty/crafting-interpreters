FMT=fmt/include
INC=$(FMT)
INC_PARAMS=$(foreach d, $(INC), -I$d)
SRCS = $(wildcard *.cpp)

build: main.cpp
	$(MAKE) clean
	g++ -g -Wall -std=c++2a $(INC_PARAMS) -DFMT_HEADER_ONLY $(SRCS) -o main

clean: 
	del *.exe