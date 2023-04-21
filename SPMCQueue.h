/*
MIT License

Copyright (c) 2020 Meng Rao <raomeng1@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#pragma once
#include <atomic>

template<class T, uint32_t CNT>
class SPMCQueue
{
public:
  static_assert(CNT && !(CNT & (CNT - 1)), "CNT must be a power of 2");
  struct Reader
  {
    operator bool() const { return q; }
    T* read() {
      auto& blk = q->blks[next_idx % CNT];
      uint32_t new_idx = ((std::atomic<uint32_t>*)&blk.idx)->load(std::memory_order_acquire);
      if (int(new_idx - next_idx) < 0) return nullptr;
      next_idx = new_idx + 1;
      return &blk.data;
    }

    T* readLast() {
      T* ret = nullptr;
      while (T* cur = read()) ret = cur;
      return ret;
    }

    SPMCQueue<T, CNT>* q = nullptr;
    uint32_t next_idx;
  };

  Reader getReader() {
    Reader reader;
    reader.q = this;
    reader.next_idx = write_idx + 1;
    return reader;
  }

  template<typename Writer>
  void write(Writer writer) {
    auto& blk = blks[++write_idx % CNT];
    writer(blk.data);
    ((std::atomic<uint32_t>*)&blk.idx)->store(write_idx, std::memory_order_release);
  }

private:
  friend class Reader;
  struct alignas(64) Block
  {
    uint32_t idx = 0;
    T data;
  } blks[CNT];

  alignas(128) uint32_t write_idx = 0;
};
