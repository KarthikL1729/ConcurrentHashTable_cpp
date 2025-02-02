#include "inc/interface_funcs_atomic.h"
#include "inc/ChainedConcurrentHT.h"
#include <thread>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <num_buckets>" << endl;
        exit(1);
    }
    int num_buckets = atoi(argv[1]);

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        cerr << "Shared memory failed" << endl;
        exit(1);
    }
    ftruncate(shm_fd, SHM_SIZE);
    SharedMemory* shared_mem = (SharedMemory*)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_mem == MAP_FAILED) {
        cerr << "Map failed" << endl;
        exit(1);
    }
    shared_mem->done = false;
    shared_mem->client_id = 0;
    shared_mem->server_id = 0;

    ConcurrentHashTable<int, int> hash_table(num_buckets);

    const int num_workers = 4;
    vector<thread> workers;
    atomic<bool> done = false;
    int num_inserts = 0;
    int num_deletes = 0;
    int num_reads = 0;
    for (int i = 0; i < num_workers; ++i) {
        workers.emplace_back([&hash_table, shared_mem, &done, &num_inserts, &num_deletes, &num_reads]() {
            while (!done.load()) {
                size_t current_server = shared_mem->server_id.load(memory_order_acquire);
                if (current_server == shared_mem->client_id.load(memory_order_acquire)) {
                    this_thread::yield();
                    continue;
                }

                Request& req = shared_mem->requests[current_server];
                bool expected = false;
                if (req.processed.compare_exchange_strong(expected, true, memory_order_acq_rel)) {
                    bool success = false;
                    switch (req.type.load(memory_order_relaxed)) {
                        case INSERT:
                            hash_table.insert(req.key.load(memory_order_relaxed), 
                                            req.value.load(memory_order_relaxed));
                            success = true;
                            num_inserts++;
                            break;
                        case READ:
                            success = hash_table.find(req.key.load(memory_order_relaxed));
                            num_reads++;
                            break;
                        case DELETE:
                            success = hash_table.remove(req.key.load(memory_order_relaxed));
                            num_deletes++;
                            break;
                    }
                    req.success.store(success, memory_order_relaxed);

                    size_t next_server = (current_server + 1) % QUEUE_SIZE;
                    shared_mem->server_id.store(next_server, memory_order_release);
                }
            }
        });
    }

    while (!shared_mem->done.load()) {
        this_thread::sleep_for(chrono::milliseconds(100));
    }
    done.store(true);

    for (auto& worker : workers) {
        worker.join();
    }

    cout << "Final state of the hash table:" << endl;
    hash_table.print();
    cout << "Number of inserts: " << num_inserts << endl;
    cout << "Number of reads: " << num_reads << endl;
    cout << "Number of deletes: " << num_deletes << endl;
    cout << "Server shutting down..." << endl;

    munmap(shared_mem, SHM_SIZE);
    close(shm_fd);
    return 0;
}