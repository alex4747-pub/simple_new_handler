# C++ Version
The required version is c++17. The code that supports
older version is 'legacy' subdirectory.

# Simple C++ New-Handler

This is a trivial include-only handler that provides meaningful functionality

1. Call Init() during program initialization  with all defaults: it will simply
   set Simple::NewHandler::std::terminate as the c++ new-handler.
   
2. Call Init() with parameters to get additional services

   -  final-block-size                         - request memory reservation to execute terminate()
   -  reserved-block-size,reserved-block-count - requested memory reservation, default none
   -  signal                                   - raise signal after release of each reserved block.
   -  allow_chain                              - if a handler already set, chain this one in front.
                                                 It does not make much sense but it is here for completness.

## Getting Started

1. Just copy include/simple_new_handler.h to an appropriate location for common include files
of your project.

2. Add call to simple::NewHandler::Init() with appropriate parameters as a part of program
initialization before starting multi-threading.

* It is OK to call this function more than once, but repeated calls will have no effect.

* The Init() is a best effort operation: it will allocate as many blocks as possible.
   However, in all cases an extra block will be allocated and then immediately
   freed to guarantee immediate memory availability.

* If the reserved-block-count is greater than UINT_MAX, UINT_MAX blocks will be allocated

3. Use state() function to retrieve the minimal state: the number of allocated and available data blocks

4. Use fullState() function to retrieve complete state, it is used mostly for diagnostics and debugging. 


### Prerequisites

Tests and examples are linux-specific because they set memory limit for the process.

### Installing

Just copy include/SimpleNewHandler.hpp to appropriate location for common include files
in your project.

## Running the tests

Do 'make test' to run tests.

### Notes

1. There are no locks. It is expected that initialization is performed before entering
multi-threaded mode, the infrastructure takes necessary protection before calling 'process'
function and for state-read purposes using int-sized variable is good enough.

2. It is small enough to be implemented as include file only. Hence the need to use worker
subclass and the singleton.

3. The Google codying style is used because Google provides formatting tools.

## Authors

* **Aleksey Romanov** - *Initial work* -


## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments



