#include <bits/stdc++.h>
#include "../SPMCQueue.h"
using namespace std;

struct Msg
{
  uint64_t tsc;
  uint64_t i;
};

inline uint64_t rdtsc() {
  return __builtin_ia32_rdtsc();
}

bool cpupin(int cpuid) {
  cpu_set_t my_set;
  CPU_ZERO(&my_set);
  CPU_SET(cpuid, &my_set);
  if (sched_setaffinity(0, sizeof(cpu_set_t), &my_set)) {
    std::cout << "sched_setaffinity error: " << strerror(errno) << std::endl;
    return false;
  }

  return true;
}

const uint64_t MaxI = 1000000;

using Q = SPMCQueue<Msg, 1024>;
Q q;

void read_thread(int tid, int cpu) {
  if (cpu >= 0) {
    cpupin(cpu);
  }
  auto reader = q.getReader();
  uint64_t cnt = 0;
  uint64_t total_lat = 0;
  while (true) {
    Msg* msg = reader.read();
    if (!msg) continue;
    auto now = rdtsc();
    auto latency = now - msg->tsc;
    total_lat += latency;
    cnt++;
    assert(msg->i >= cnt);
    if (msg->i == MaxI) break;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(tid));
  cout << "tid: " << tid << ", drop cnt: " << (MaxI - cnt) << ", avg lat: " << (total_lat / cnt) << endl;
}

// usage: ./multhread [reader cnt] [first cpu to bind]
int main(int argc, char** argv) {
  int reader_cnt = 3;
  int cpu_start = -1;
  if (argc >= 2) {
    reader_cnt = stoi(argv[1]);
  }
  if (argc >= 3) {
    cpu_start = stoi(argv[2]);
  }
  vector<thread> reader_thrs;
  for (int i = 0; i < reader_cnt; i++) {
    reader_thrs.emplace_back(read_thread, i, cpu_start < 0 ? -1 : cpu_start + i);
  }

  if (cpu_start >= 0) {
    cpupin(cpu_start + reader_cnt);
  }

  for (uint64_t i = 1; i <= MaxI; i++) {
    q.write([i](Msg& msg) {
      msg.i = i;
      msg.tsc = rdtsc();
    });
    auto expire = rdtsc() + 1000;
    while (rdtsc() < expire) continue;
  }

  for (auto& thr : reader_thrs) {
    thr.join();
  }
  return 0;
}

