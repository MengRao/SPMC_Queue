#include <bits/stdc++.h>
#include "shm.h"
using namespace std;

// usage: ./shm_reader [shm file]
// use taskset -c to bind core
int main(int argc, char** argv) {
  const char* shm_file = "SPMCQueue_test";
  if (argc >= 2) {
    shm_file = argv[1];
  }
  Q* q = shmmap(shm_file);
  if (!q) return 1;
  auto reader = q->getReader();
  cout << "reader size: " << sizeof(reader) << endl;

  while (true) {
    Msg* msg = reader.readLast();
    // Msg* msg = reader.read();
    if (!msg) continue;
    auto now = rdtsc();
    auto latency = now - msg->tsc;
    cout << "i: " << msg->i << ", latency: " << latency << endl;
  }

  return 0;
}


