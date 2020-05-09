# Simple C++ New-Handler

This is a trivial include-only handler that provides meaningful functionality

1. Call init() during program initialization  with all defaults: it will simply
   set Simple::NewHandler::std::terminate and c++ new-handler.
   
2. Call init() with parameters to get additional services

   -  finalBlockSize                       - request memory reservation to execute terminate()
   -  reservedBlockSize,reservedBlockCount - requested memory reservation, default none
   -  terminateReserveSize                 - requested memory reservation to execute terminate()
   -  signal                               - raise signal after release of each reserved block.

## Getting Started

1. Just copy include/SimpleNewHandler.h to appropriate location for common include files
of your project.

2. Add call to Simple::NewHandler::init() with appropriate parameters as a part of program
initialization before starting multi-threading.

...It is OJ to call this function more than once, but repeated calls have no effect.

...The init() is a best effort operation: it will allocate as many blocks as possible
...If the reserved-block-count is greater than UINT_MAX, UINT_MAX blocks will be allocated

3. Use state() function to retrieve the minimal state: the number of allocated and available data blocks

4. Use fullState() function to retrieve complete state, it is used mostly for diagnostics and debugging. 


### Prerequisites

There are no prerequisites

### Installing

Just copy include/SimpleNewHandler.hpp to appropriate location for common include files
in your project.

## Running the tests

Do 'make test' to run tests.

### Notes

1. There are no locks. It is expected that initialization is performed before entering
multi-threaded mode, the infrastructure takes necessary protection before calling process and
for state read purposes volatile int variable is good enough.

2. It is small enough to be implemented as include file only. Hence the need to use worker
subclass and the singleton.

## Authors

* **Aleksey Romanov** - *Initial work* -


## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments



