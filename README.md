## SPMCQueue
SPMCQueue is actually a single publisher(writer) multiple subscriber(reader) queue C++ template class, which can be easily used for inter-thread or inter-process communication.

A key feature of this queue is that the writer won't wait if one of the readers is far behind, in which case we noramlly call a queue being "full". Actually there is no concept of being "full" in SPMC_Queue, the writer just keep writing without blocking or failure, it's not even aware of the existence of the readers. It's the reader's responsibility to not fall behind too much, if a reader is slower by more than queue size it'll start dropping messages. This is very simalar to udp multicasting in the network, where the sender doesn't care the receivers who subscribed the group and just send packages in its own speed, if a receiver is slow the NIC will drop packages when its incoming buffer is full. Likeyly, SPMC_Queue is "multicasting" within a host among threads or processes, in a pretty efficent way.

## Usage
A SPMCQueue class is defined as `SPMCQueue<T, CNT>` where `T` is user msg type and `CNT` is the queue size; In the following example the queue is defined as 
```c++ 
SPMCQueue<int, 1024> q;
```

The writer use a lambda callback function to write directly into the queue memory, making the write operation "zero-copy" and always successful:
```c++
q.write([](int& msg){
  msg = 123;
});
```

For readers, initially one reader needs to get a `Reader` object from the queue:
```c++
  auto reader = q.getReader();
```

The read operation is non-blocking, which means user needs to repetitively call the `read()`/`readLast()` function to poll new messages from the writer. `read()`/`readLast()` returns `T*` if it succeeds or `nullptr` if no new message:
```c++
  int* msg = reader.read();
  if (msg) {
    std::cout << "msg: "<< *msg << std::endl;
  }
```

The read/readLast operation is also zero-copy: it simply returns the address of the next or the last object the writer has written, but the user should not save the reference for too long as it could be rewritten by the writer, it's better to copy the object for user himself if its content needs to be saved.

## Examples
There are example codes in example dir for both ITC and IPC usage.

## Performance
~~Don't bother, there is no more efficient implementation in the world.~~

Use example/multhread.cc for benchmark, on a host with "Intel(R) Xeon(R) Gold 6144 CPU @ 3.50GHz" and cpu isolated and pinned, in one reader scenario, average latency is 335 tsc(95.7 ns), and one additional reader makes the latency 40 tsc(11.4 ns) higher.

## Multiple Producer Version?
Actually SPMCQueue can be easily altered to supoort multiple producer by replacing `++write_idx` with atomic fetch add. But multiple producer queue is a bad design because producers' contention deteriorates performance(in this case producers contend on write_idx). It's better to use multiple SPMCQueue, each for one producer thread, and have consumers poll on all these queues.

## An Implementation for Dynamic-Sized Msg
If you're looking for a SPMC queue which can handle messages with dynamic size(like a real multicasting udp package), check [PubSubQueue](https://github.com/MengRao/PubSubQueue).
