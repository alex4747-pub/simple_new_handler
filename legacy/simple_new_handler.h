// Copyright (C) 2020  Aleksey Romanov (aleksey at voltanet dot io)
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

// NOLINT(build/header_guard)
//
#ifndef INCLUDE_SIMPLE_NEW_HANDLER_H_
#define INCLUDE_SIMPLE_NEW_HANDLER_H_

#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <limits>
#include <new>
#include <utility>

// Let us make use of modern c++ functionality
// while being backward compatible with classic
//
// If NOEXCEPT is already defined hopefully it has
// correct value
//
#ifndef NOEXCEPT
#define NOEXCEPT_DEFINED 1
#if __cplusplus >= 201103L
#define NOEXCEPT noexcept
#else
#define NOEXCEPT
#endif
#endif

namespace simple {

class NewHandler {
 public:
  // Initialize the driver and allocate reserved memory blocks
  //
  // If not enough memory allocate as many blocks as possible
  //
  static void Init(size_t final_block_size = 0, size_t reserved_block_count = 0,
                   size_t reserved_block_size = 0, int signo = 0,
                   bool allow_chain = false) NOEXCEPT {
    GetWorker().Init(final_block_size, reserved_block_count,
                     reserved_block_size, signo, allow_chain);
  }

  // Basic state
  //
  struct State {
    State() NOEXCEPT : allocated_block_count(0), available_block_count(0) {}

    size_t allocated_block_count;
    size_t available_block_count;
  };

  // Full state
  //
  struct FullState {
    FullState() NOEXCEPT : init_done(),
                           signo(),
                           final_block_size(),
                           final_block_allocated(),
                           reserved_block_size(),
                           reserved_block_count(),
                           state() {}
    State GetState() const NOEXCEPT { return state; }
    bool init_done;
    bool chained;
    int signo;
    size_t final_block_size;
    bool final_block_allocated;
    size_t reserved_block_size;
    size_t reserved_block_count;
    State state;
  };

  static State GetState() NOEXCEPT { return GetWorker().GetState(); }

  static FullState GetFullState() NOEXCEPT {
    return GetWorker().GetFullState();
  }

 private:
  class Worker;

  static void Process() NOEXCEPT { GetWorker().Process(); }

  static Worker& GetWorker() NOEXCEPT {
    static Worker worker_;
    return worker_;
  }

  class Worker {
   public:
    Worker() NOEXCEPT : full_state_(),
                        available_block_count_(),
                        final_block_(),
                        blk_arr_list_(),
                        prev_handler_() {}

    // Note on concurrency:
    //
    // It is a responsibility of the user to call init()
    // before entering multithreaded environment
    //
    // The environment guarantees that
    // the handler call itself is thread safe.
    //
    // We do not care about race condition between
    // setting and reading available_block_count
    // so using int-sized integer provides enough
    // atomicity, to return either value before or
    // value after the change.
    //
    void Init(size_t final_block_size = 0, size_t reserved_block_count = 0,
              size_t reserved_block_size = 0, int signo = 0,
              bool allow_chain = false) NOEXCEPT {
      if (full_state_.init_done) {
        // We expect to be done once and it is done
        // more than once we do not care much
        return;
      }

      full_state_.init_done = true;
      full_state_.signo = signo;
      full_state_.final_block_size = final_block_size;
      full_state_.reserved_block_count = reserved_block_count;
      full_state_.reserved_block_size = reserved_block_size;

      size_t finalSize =
          (final_block_size + sizeof(Blk) - 1) / sizeof(Blk) * sizeof(Blk);

      if (finalSize) {
        final_block_ = new (std::nothrow) Blk[finalSize / sizeof(Blk)];

        if (final_block_) {
          full_state_.final_block_allocated = true;

          // Assign a value to map allocated block
          //
          final_block_->m_next = 0;
        }
      }

      unsigned int block_limit = std::numeric_limits<unsigned int>::max();

      // We always allocate and immediately free extra block
      // to support the strategy of managing all available
      // memory through this mechanism
      //
      if ((reserved_block_count + 1) < block_limit)
        block_limit = static_cast<unsigned int>(reserved_block_count + 1);

      if (reserved_block_count && reserved_block_size) {
        size_t reserved_arr_size =
            (reserved_block_size + sizeof(Blk) - 1) / sizeof(Blk);

        // We have to count actually allocated blocks
        size_t arr_count = 0;

        for (unsigned int ii = 0; ii < block_limit; ii++) {
          Blk* blk_arr = new (std::nothrow) Blk[reserved_arr_size];

          if (!blk_arr) {
            break;
          }

          arr_count++;

          blk_arr[0].m_next = blk_arr_list_;
          blk_arr_list_ = blk_arr;
        }

        if (blk_arr_list_) {
          // Immediately release the last block
          //
          Blk* blk_arr = blk_arr_list_;
          blk_arr_list_ = blk_arr[0].m_next;

          delete[] blk_arr;

          full_state_.state.allocated_block_count = arr_count - 1;
          full_state_.state.available_block_count = arr_count - 1;
          available_block_count_ = arr_count - 1;
        }
      }

      if (!allow_chain) {
        std::set_new_handler(NewHandler::Process);
        return;
      }

      prev_handler_ = std::set_new_handler(NewHandler::Process);

      if (prev_handler_) {
        full_state_.chained = true;
      }
    }

    State GetState() const NOEXCEPT { return full_state_.GetState(); }

    FullState GetFullState() const NOEXCEPT { return full_state_; }

    void Process() NOEXCEPT {
      Blk* blk_arr = blk_arr_list_;

      if (blk_arr) {
        // Release the first avalable block to the process
        // and raise signal if configured
        blk_arr_list_ = blk_arr[0].m_next;

        delete[] blk_arr;

        if (available_block_count_ > 0) {
          available_block_count_--;
          full_state_.state.available_block_count = available_block_count_;
        }

        if (full_state_.signo != 0) std::raise(full_state_.signo);

        return;
      }

      // Release final block and terminate or call chained handler
      delete[] final_block_;
      final_block_ = 0;

      if (prev_handler_) {
        std::set_new_handler(prev_handler_);
        prev_handler_();
      } else {
        std::terminate();
      }
    }

   private:
    struct Blk {
      Blk* m_next;
    };

    FullState full_state_;
    unsigned int available_block_count_;
    Blk* final_block_;
    Blk* blk_arr_list_;
    std::new_handler prev_handler_;
  };
};
}  // namespace simple

// If noexcept was defined in this
// file, clean it up
//
#ifdef NOEXCEPT_DEFINED
#undef NOEXCEPT
#undef NOEXCEPT_DEFINED
#endif

#endif  // INCLUDE_SIMPLE_NEW_HANDLER_H_
