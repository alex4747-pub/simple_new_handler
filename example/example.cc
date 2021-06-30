// Copyright (C) 2020  Aleksey Romanov
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom
// the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//
// Example program for sane new handler
//

#include <getopt.h>
#include <simple_new_handler.h>
#include <sys/resource.h>
#include <sys/time.h>

#include <cassert>
#include <csignal>
#include <exception>
#include <iostream>
#include <new>
#include <utility>

static size_t const MB = 1024 * 1024;
static size_t alloc_count = 0;

static void TerminateHandler() {
  // Do normal exit instead of abort
  std::cout << "Terminated at " << (alloc_count + 1) << " MB\n";
  exit(0);
}

// The example is simple
//
// 1. Configure low memory limit
// 2. Init the handler
// 3. Simulate a memory leak
// 4. Make sure terminate was called.
//
int main(int, char**) {
  //////////////////////////////////////////////////////
  // Set terminate handler to print reached allocation level
  std::set_terminate(TerminateHandler);

  ////////////////////////////////////////////////////
  // Set memory limit at 100MB
  size_t const limit = 100;

  std::cout << "Memory limit: " << limit << "MB\n";

  // Set memory limit
  rlimit rl = {limit * MB, limit * MB};

  int res = setrlimit(RLIMIT_AS, &rl);
  assert(res == 0);

  ////////////////////////////////////////////////////
  // Initialize driver with 6 x 10 MB reserve blocks
  simple::NewHandler::Init(1024, 6, 10 * MB);

  ///////////////////////////////////////////////////
  // Check initial state
  auto fullState = simple::NewHandler::GetFullState();

  assert(fullState.init_done);
  assert(fullState.signo == 0);
  assert(fullState.chained == false);
  assert(fullState.final_block_size == 1024);
  assert(fullState.final_block_allocated);
  assert(fullState.reserved_block_size == 10 * MB);
  assert(fullState.reserved_block_count == 6);
  assert(fullState.state.allocated_block_count == 6);
  assert(fullState.state.available_block_count ==
         fullState.state.allocated_block_count);

  ////////////////////////////////////////////////////////////
  // Prinit initial state
  simple::NewHandler::State state = simple::NewHandler::GetState();

  std::cout << "Available " << state.available_block_count
            << " blocks, 10MB each\n";

  size_t avail = state.available_block_count;

  ////////////////////////////////////////////////////////////
  // Simulate memory leak
  try {
    for (alloc_count = 0; alloc_count < 10000000; alloc_count++) {
      char* p = new char[1024 * 1024];
      (void)p;

      std::cout << "Allocated " << (alloc_count + 1) << " MB\n";

      simple::NewHandler::State state = simple::NewHandler::GetState();

      if (state.available_block_count < avail) {
        std::cout << "Block " << (state.allocated_block_count - avail)
                  << " released at " << (alloc_count + 1) << " MB\n";

        avail = state.available_block_count;
      }
    }
  } catch (std::bad_alloc& e) {
    // Should not be here
    assert(false);
    return 1;
  }

  // Should not be here
  assert(false);
  return 2;
}
