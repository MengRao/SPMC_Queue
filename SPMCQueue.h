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

template<class T, unsigned int CNT>
class SPMCQueue
{
public:
  static_assert(CNT && !(CNT & (CNT - 1)), "CNT must be a power of 2");
  struct Reader
  {
    operator bool() const { return q; }
    T* read() {
      auto& blk = q->blks[(idx + 1) % CNT];
      asm volatile("" : "=m"(blk) : :);
      unsigned int new_idx = blk.idx;
      if ((int)new_idx - (int)idx <= 0) return nullptr;
      idx = new_idx;
      return &blk.data;
    }

    T* readLast() {
      unsigned int last_idx = *(volatile unsigned int*)&q->write_idx;
      if ((int)last_idx - (int)idx <= 0) return nullptr;
      idx = last_idx;
      return &q->blks[last_idx % CNT].data;
    }

    SPMCQueue<T, CNT>* q = nullptr;
    unsigned int idx;
  };

  Reader getReader() {
    Reader reader;
    reader.q = this;
    reader.idx = write_idx;
    return reader;
  }

  template<typename Writer>
  void write(Writer writer) {
    auto& blk = blks[(write_idx + 1) % CNT];
    writer(blk.data);
    asm volatile("" : : "m"(blk) :);
    blk.idx = ++write_idx;
    asm volatile("" : : "m"(write_idx), "m"(blk) :);
  }

private:
  friend class Reader;
  struct alignas(64) Block
  {
    unsigned int idx = 0;
    T data;
  } blks[CNT];

  alignas(128) unsigned int write_idx = 0;
};
