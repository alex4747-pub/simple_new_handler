.PHONY: all test clean formam tidy cpplint

FORMAT   = clang-format
TIDY     = clang-tidy
CPPLINT = cpplint

all:

test:
	cd test; $(MAKE) run-test

clean:
	rm -rf *~
	cd test; $(MAKE) clean

format:
	$(FORMAT) --style=google -i ./simple_new_handler.h
	cd test; $(MAKE) format

tidy:
	$(TIDY) --fix -extra-arg-before=-xc++ ./simple_new_handler.h --  -std=c++11
	cd test; $(MAKE) tidy

cpplint:
	$(CPPLINT) ./simple_new_handler.h
	cd test; $(MAKE) cpplint



