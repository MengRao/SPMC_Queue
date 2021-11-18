#include <bits/stdc++.h>
#include "shm.h"
using namespace std;

// usage: ./shm_write [shm file]
// use taskset -c to bind core
int main(int argc, char** argv) {
  const char* shm_file = "SPMCQueue_test";
  if (argc >= 2) {
    shm_file = argv[1];
  }
  Q* q = shmmap(shm_file);
  if (!q) return 1;

  uint64_t i = 0;
  while (true) {
    for (int _ = 0; _ < 3; _++) {
      q->write([i](Msg& msg) {
        msg.i = i;
        msg.tsc = rdtsc();
      });
      i++;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  return 0;
}

