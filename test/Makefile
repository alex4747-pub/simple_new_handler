STD=-std=c++11
CXXFLAGS = -g -O0 -I../include

USE_GCC=

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

all: test_simple_new_handler test_simple_new_handler_std

test_simple_new_handler: test_simple_new_handler.cc ../include/simple_new_handler.h Makefile
	$(CXX) -o $@ $(CXXFLAGS) $< $(LIBS)

test_simple_new_handler_std: test_simple_new_handler.cc ../include/simple_new_handler.h Makefile
	$(CXX) -o $@ $(CXXFLAGS) $(STD) $< $(LIBS)

format:
	$(FORMAT) --style=google -i test_simple_new_handler.cc

tidy:
	$(TIDY) --fix -extra-arg-before=-xc++ test_simple_new_handler.cc ../include/simple_new_handler.h -- $(CXXFLAGS) $(STD)

cpplint:
	$(CPPLINT) test_simple_new_handler.cc ../include/simple_new_handler.h

clean:
	rm -rf test_simple_new_handler test_simple_new_handler_std *~ *.dSYM

run-test: test_simple_new_handler test_simple_new_handler_std
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
	@echo "Std-test with all defaults"
	./test_simple_new_handler_std
	@echo
	@echo "Test with signal"
	./test_simple_new_handler -s
	@echo
	@echo "Std-test with signal"
	./test_simple_new_handler_std -s
	@echo
	@echo "Test with chain and debug"
	./test_simple_new_handler -c --debug
	@echo


