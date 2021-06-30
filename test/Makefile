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

all: test_simple_new_handler

test_simple_new_handler: test_simple_new_handler.cc ../simple_new_handler.h Makefile
	$(CXX) -o $@ $(CXXFLAGS) $(STD) $< $(LIBS)

format:
	$(FORMAT) --style=google -i test_simple_new_handler.cc

tidy:
	$(TIDY) --fix -extra-arg-before=-xc++ test_simple_new_handler.cc ../simple_new_handler.h -- $(CXXFLAGS) $(STD)

cpplint:
	$(CPPLINT) test_simple_new_handler.cc ../simple_new_handler.h

clean:
	rm -rf test_simple_new_handler *~ *.dSYM

# Note: test-with-debug and very small memory
# handles case where no blocks could be allocated
run-test: test_simple_new_handler
	@echo
	@echo "Test with all defaults"
	./test_simple_new_handler
	@echo
	@echo "Test with debug"
	./test_simple_new_handler --debug
	@echo
	@echo "Test with debug and small memory"
	./test_simple_new_handler --debug 40
	@echo
	@echo "Test with debug and very small memory"
	./test_simple_new_handler --debug 10
	@echo
	@echo "Test with signal"
	./test_simple_new_handler -s
	@echo
	@echo "Test with chain and debug"
	./test_simple_new_handler -c --debug
	@echo


