STD=-std=c++17
CXXFLAGS = -g -O0 -I.. -Wall -Wextra -Werror

USE_GCC=yes

ifeq ($(USE_GCC),)
CXX = clang++
LIBS = -lc++
else
CXX = g++
LIBS = -lstdc++
endif

FORMAT  = clang-format
TIDY    = clang-tidy
CPPLINT = cpplint

all: example

example: example.cc ../simple_new_handler.h Makefile
	$(CXX) -o $@ $(CXXFLAGS) $(STD) $< $(LIBS)

format:
	$(FORMAT) --style=google -i example.cc

tidy:
	$(TIDY) --fix -extra-arg-before=-xc++ example.cc ../simple_new_handler.h -- $(CXXFLAGS) $(STD)

cpplint:
	$(CPPLINT) example.cc ../simple_new_handler.h

clean:
	rm -rf example *~ *.dSYM

run: example
	./example



