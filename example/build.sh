g++ -O3 -o multhread multhread.cc -pthread
g++ -O3 -o shm_writer shm_writer.cc -lrt
g++ -O3 -o shm_reader shm_reader.cc -lrt
