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
// Test program for sane new handler
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
static bool do_chain = false;
static bool debug = false;
static bool have_signal = false;

static void TerminateHandler() {
  // Do normal exit instead of abort
  assert(!do_chain);
  if (debug) {
    std::cout << "Terminated at " << (alloc_count + 1) << " MB\n";
  }

  exit(0);
}

static void ChainedHandler() {
  assert(do_chain);
  if (debug) {
    std::cout << "Chained handler at " << (alloc_count + 1) << " MB\n";
  }

  exit(0);
}

static void SignalHandler(int signo) {
  assert(signo == SIGUSR1);
  have_signal = true;
}

static void usage() {
  std::cout << "usage: test_simple_new__handler [--debug][--signal] [--chain] "
               "[memory-limit-in-mbs]\n";
  std::cout << "\n";
}

int main(int argc, char** argv) {
  size_t limit = 200;
  int signo = 0;

#if __APPLE__
  // We cannot run this test on macos because it ignores
  // setrlimit settings
  //
  std::cout << "We cannot run this test on MacOS" << std::endl;
  return 1;
#endif

  static struct option long_options[] = {{"chain", no_argument, 0, 1},
                                         {"debug", no_argument, 0, 2},
                                         {"help", no_argument, 0, 3},
                                         {"signal", no_argument, 0, 4},
                                         {0, 0, 0, 0}};

  for (;;) {
    int c = getopt_long(argc, argv, "cdhs", long_options, 0);

    if (c < 0) {
      break;
    }

    switch (c) {
      case 1:
      case 'c':
        do_chain = true;
        break;

      case 2:
      case 'd':
        debug = true;
        break;

      case 3:
      case 'h':
        usage();
        return 0;

      case 4:
      case 's':
        signo = SIGUSR1;
        break;

      default:
        usage();
        return 1;
    }
  }

  if (optind < argc) {
    if ((optind + 1) < argc) {
      std::cout << "too many parameters\n";
      usage();
      return 1;
    }

    char* e = 0;
    size_t tmp = strtoul(argv[optind], &e, 0);

    if (e == 0 || *e != 0 || tmp == 0) {
      std::cout << "bad memory limit\n";
      usage();
      return 1;
    }

    limit = tmp;
  }

  if (debug) {
    std::cout << "Memory limit: " << limit << "MB\n";
  }

  std::signal(signo, SignalHandler);

  // Set 200MB limit
  rlimit rl = {limit * MB, limit * MB};

  int res = setrlimit(RLIMIT_AS, &rl);
  assert(res == 0);

  simple::NewHandler::FullState fullState = simple::NewHandler::GetFullState();

  assert(!fullState.init_done);
  assert(fullState.signo == 0);
  assert(fullState.final_block_size == 0);
  assert(!fullState.final_block_allocated);
  assert(fullState.reserved_block_size == 0);
  assert(fullState.reserved_block_count == 0);
  assert(fullState.state.allocated_block_count == 0);
  assert(fullState.state.available_block_count == 0);

  if (do_chain) {
    std::set_new_handler(ChainedHandler);
  }

  // Init with 10 spare chunks
  // 10 MB each cnhunk
  // and 1K reserve
  simple::NewHandler::Init(1024, 10, 10 * MB, signo, do_chain);

  // Set terminate handler to print reached allocation level
  std::set_terminate(TerminateHandler);

  fullState = simple::NewHandler::GetFullState();

  assert(fullState.init_done);
  assert(fullState.signo == signo);
  assert(fullState.chained == do_chain);
  assert(fullState.final_block_size == 1024);
  assert(fullState.final_block_allocated);
  assert(fullState.reserved_block_size == 10 * MB);
  assert(fullState.reserved_block_count == 10);
  assert(fullState.state.allocated_block_count <= 10);
  assert(fullState.state.available_block_count ==
         fullState.state.allocated_block_count);

  simple::NewHandler::State state = simple::NewHandler::GetState();

  if (debug) {
    std::cout << "Available " << state.available_block_count
              << " blocks, 10MB each\n";
  }

  size_t avail = state.available_block_count;

  try {
    for (alloc_count = 0; alloc_count < 10000000; alloc_count++) {
      char* p = new char[1024 * 1024];
      (void)p;

      if (debug) {
        std::cout << "Allocated " << (alloc_count + 1) << " MB\n";
      }

      simple::NewHandler::State state = simple::NewHandler::GetState();

      if (state.available_block_count < avail) {
        if (debug) {
          std::cout << "Block " << (state.allocated_block_count - avail)
                    << " released at " << (alloc_count + 1) << " MB\n";

          // We should get a signal if configured
          if (signo != 0) {
            assert(have_signal);
            have_signal = false;
          }
        }
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
