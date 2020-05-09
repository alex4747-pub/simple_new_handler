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

// NOLINT(build/header_guard)

#ifndef _INCLUDE_SIMPLE_NEW_HANDLER_H_
#define _INCLUDE_SIMPLE_NEW_HANDLER_H_

#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <limits>
#include <new>
#include <utility>

namespace simple {

class NewHandler {
 public:
  // Initialize the driver and allocate reserved memory blocks
  //
  // If not enough memory allocate as many blocks as possible
  //
  static void Init(size_t finalBlockSize = 0, size_t reservedBlockCount = 0,
                   size_t reservedBlockSize = 0, int signo = 0) {
    GetWorker().Init(finalBlockSize, reservedBlockCount, reservedBlockSize,
                     signo);
  }

  struct State {
    size_t allocated_block_count;
    size_t available_block_count;
  };

  static State GetState() { return GetWorker().GetState(); }

  // Full state
  struct FullState {
    bool init_done;
    int signo;
    size_t final_block_size;
    bool final_block_allocated;
    size_t reserved_block_size;
    size_t reserved_block_count;
    size_t allocated_block_count;
    size_t available_block_count;
  };

  static FullState GetFullState() { return GetWorker().GetFullState(); }

 private:
  class Worker;

  static void Process() { GetWorker().Process(); }

  static Worker& GetWorker() {
    static Worker worker_;
    return worker_;
  }

  class Worker {
   public:
    Worker()
        : init_done_(),
          signo_(),
          final_block_size_(),
          reserved_block_size_(),
          reserved_block_count_(),
          allocated_block_count_(),
          available_block_count_(),
          final_block_(),
          blk_arr_list_() {}

    // Note on concurrency:
    //
    // It is a responsibility of the user to call init()
    // before entering multithreaded environment
    //
    // The environment guarantees that
    // the handler call itself is thread safe.
    //
    // Volatile is good enough for reporting purposes
    // because we do not care about race condition
    // between actual and reported usage
    //
    void Init(size_t finalBlockSize = 0, size_t reservedBlockCount = 0,
              size_t reservedBlockSize = 0, int signo = 0) {
      if (init_done_) {
        // We expect to be done once and it is done
        // more than once we do not care much
        return;
      }

      final_block_size_ = finalBlockSize;

      size_t finalSize =
          (final_block_size_ + sizeof(Blk) - 1) / sizeof(Blk) * sizeof(Blk);

      if (finalSize) {
        final_block_ = new (std::nothrow) Blk[finalSize / sizeof(Blk)];

        if (final_block_) {
          // Assign a value to map allocated block
          final_block_->m_next = 0;
        }
      }

      reserved_block_count_ = reservedBlockCount;

      unsigned int blockLimit = std::numeric_limits<unsigned int>::max();

      if (reserved_block_count_ < blockLimit)
        blockLimit = static_cast<unsigned int>(reserved_block_count_);

      reserved_block_size_ = reservedBlockSize;

      if (reserved_block_count_ && reserved_block_size_) {
        size_t reservedSize = (reserved_block_size_ + sizeof(Blk) - 1) /
                              sizeof(Blk) * sizeof(Blk);

        for (unsigned int ii = 0; ii < blockLimit; ii++) {
          Blk* blkArr = new (std::nothrow) Blk[reservedSize / sizeof(Blk)];

          if (!blkArr) {
            allocated_block_count_ = ii;
            break;
          }

          blkArr[0].m_next = blk_arr_list_;
          blk_arr_list_ = blkArr;
        }

        if (blk_arr_list_ && !allocated_block_count_) {
          // All reserved blocks were allocated
          allocated_block_count_ = blockLimit;
        }

        available_block_count_ = allocated_block_count_;
      }

      signo_ = signo;

      std::set_new_handler(NewHandler::Process);

      init_done_ = true;
    }

    State GetState() const volatile {
      State s;

      memset(&s, 0, sizeof(s));

      if (!init_done_) {
        return s;
      }

      s.allocated_block_count = allocated_block_count_;
      s.available_block_count = available_block_count_;

      return s;
    }

    FullState GetFullState() const volatile {
      FullState s;

      memset(&s, 0, sizeof(s));

      if (!init_done_) {
        return s;
      }

      s.init_done = true;
      s.signo = signo_;
      s.final_block_size = final_block_size_;
      if (final_block_) s.final_block_allocated = true;
      s.reserved_block_size = reserved_block_size_;
      s.reserved_block_count = reserved_block_count_;
      s.allocated_block_count = allocated_block_count_;
      s.available_block_count = available_block_count_;

      return s;
    }

    void Process() {
      Blk* curBlkArr = blk_arr_list_;

      if (curBlkArr) {
        // Release the first avalable block to the process
        // and raise signal if configured
        blk_arr_list_ = curBlkArr[0].m_next;

        delete[] curBlkArr;

        if (available_block_count_ > 0) available_block_count_--;

        if (signo_ != 0) std::raise(signo_);

        return;
      }

      // Release final block and terminate
      delete[] final_block_;
      final_block_ = 0;
      std::terminate();
    }

   private:
    struct Blk {
      Blk* m_next;
    };

    bool init_done_;
    int signo_;
    size_t final_block_size_;
    size_t reserved_block_size_;
    size_t reserved_block_count_;
    unsigned int allocated_block_count_;
    volatile unsigned int available_block_count_;
    Blk* final_block_;
    Blk* blk_arr_list_;
  };
};
}  // namespace simple

#endif  // _INCLUDE_SIMPLE_NEW_HANDLER_H_
