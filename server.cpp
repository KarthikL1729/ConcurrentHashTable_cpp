#include "inc/interface_funcs.h"
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
    
    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&shared_mem->mtx, &mutex_attr);
    pthread_mutexattr_destroy(&mutex_attr);

    pthread_mutex_lock(&shared_mem->mtx);
    shared_mem->done = false;
    shared_mem->client_id = 0;
    shared_mem->server_id = 0;
    pthread_mutex_unlock(&shared_mem->mtx);

    pthread_condattr_t cond_attr;
    pthread_condattr_init(&cond_attr);
    pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&shared_mem->shutdown_cond, &cond_attr);
    pthread_condattr_destroy(&cond_attr);

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
                pthread_mutex_lock(&shared_mem->mtx);
                size_t current_server = shared_mem->server_id;
                size_t client = shared_mem->client_id;

                if (current_server == client) { // Queue is empty
                    pthread_mutex_unlock(&shared_mem->mtx);
                    this_thread::yield();
                    continue;
                }

                Request& req = shared_mem->requests[current_server];
                if (!req.processed) {
                    req.processed = true;
                    bool success = false;
                    switch (req.type) {
                        case INSERT:
                            hash_table.insert(req.key, req.value);
                            success = true;
                            num_inserts++;
                            break;
                        case READ:
                            hash_table.find(req.key);
                            num_reads++;
                            break;
                        case DELETE:
                            hash_table.remove(req.key);
                            num_deletes++;
                            break;
                    }
                    req.success = success;
                    shared_mem->server_id = (current_server + 1) % QUEUE_SIZE;
                }
                pthread_mutex_unlock(&shared_mem->mtx);
            }
        });
    }

    pthread_mutex_lock(&shared_mem->mtx);
    while (!shared_mem->done) {
        pthread_cond_wait(&shared_mem->shutdown_cond, &shared_mem->mtx); 
    }
    pthread_mutex_unlock(&shared_mem->mtx);
    done.store(true);

    for (auto& worker : workers) {
        worker.join();
    }

    cout << "Final state of the hash table:" << endl;
    //hash_table.print();
    cout << "Number of inserts: " << num_inserts << endl;
    cout << "Number of reads: " << num_reads << endl;
    cout << "Number of deletes: " << num_deletes << endl;
    cout << "Server shutting down..." << endl;

    munmap(shared_mem, SHM_SIZE);
    close(shm_fd);
    return 0;
}