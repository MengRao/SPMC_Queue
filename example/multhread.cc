#include <bits/stdc++.h>
#include "../SPMCQueue.h"
#include "Statistic.h"
using namespace std;

struct Msg
{
  uint64_t tsc;
  uint64_t i[1];
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
  Statistic<uint64_t> stats;
  stats.reserve(MaxI);
  while (true) {
    Msg* msg = reader.read();
    if (!msg) continue;
    auto now = rdtsc();
    auto latency = now - msg->tsc;
    stats.add(latency);
    cnt++;
    assert(msg->i[0] >= cnt);
    for (auto cur : msg->i) assert(cur == msg->i[0]);
    if (msg->i[0] == MaxI) break;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(tid * 100));
  cout << "tid: " << tid << ", drop cnt: " << (MaxI - cnt) << ", latency stats: " << endl;
  stats.print(cout);
  cout << endl;
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

  std::this_thread::sleep_for(std::chrono::milliseconds(1));

  if (cpu_start >= 0) {
    cpupin(cpu_start + reader_cnt);
  }

  for (uint64_t i = 1; i <= MaxI; i++) {
    q.write([i](Msg& msg) {
      for (auto& cur : msg.i) cur = i;
      msg.tsc = rdtsc();
    });
    // try set smaller waiting time to increase miss cnt, and set ZERO_COPY_READ = true to see if assert could fail
    auto expire = rdtsc() + 1000;
    while (rdtsc() < expire) continue;
  }

  for (auto& thr : reader_thrs) {
    thr.join();
  }
  return 0;
}

