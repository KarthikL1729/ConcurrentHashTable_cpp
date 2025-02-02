# Concurrent Hash Table in C++

This is a simple implementation of a concurrent hash table in C++. The hash table is implemented using a fixed-size vector of linked lists (each linked list is a bucket). Each linked list is protected by a reader-writer lock (implemented using a `shared_mutex` object), which ensures that only one thread can access a linked list at a time. Collisions are resolved by chaining. The hash table supports read, write and delete operations. 

## Usage

To use the hash table, the `ChainedConcurrentHT.h` header file in the `inc` directory needs to be included. The Hash table can be created given a size (number of buckets). The current implementation of the hash table can store values of any data type. 

- Declaration: `ConcurrentHashTable<int, int> hash_table(num_buckets)`
- Member functions:
    - `insert`: Inserts an key-value pair
    - `find`: Searches the chain (if any) for a given key and prints value
    - `remove`: Deletes key-value pair given a key
    - `print`: Prints the current state of the hash table

## Tasks

### Server program

The Server program (`server.cpp`) creates a `ConcurrentHashTable` object given a number of buckets. It then opens a shared memory object (guarded by a mutex to allow for multithreading) and runs in an loop until the `shared_mem->done` object is set to true. The shared memory object has the following structure (including the `Request` struct and `RequestType` enums):

```cpp
enum RequestType {
    INSERT,
    READ,
    DELETE
};

struct Request {
    RequestType type;
    int key;
    int value;
    bool processed;
    bool success;
};

struct SharedMemory {
    bool done;
    Request requests[QUEUE_SIZE];
    size_t client_id;
    size_t server_id;
    pthread_mutex_t mtx;
    pthread_cond_t shutdown_cond;
};
```

The shared memory object has a fixed-size queue that is used as a ring-buffer to store requests from the client program (to ensure that the threads aren't forced to sequentially execute). `client_id` and `server_id` are the position indices in the queue where the client and server can enqueue and dequeue requests. The server program is multithreaded, and runs a number of worker threads (specified in the code, currently 4), which can independently process requests from the queue. This is thread-safe because of the mutex guarding the shared memory object.

In each iteration, the server checks the shared memory for enqueued operations and processes them. The server program must be compiled and run as follows:

```bash
g++ server.cpp -o server -std=c++17 -lrt -pthread -fsanitize=thread
./server <num_buckets>
```

The server program must be run before running the client program.

### Client programs

The main client program `client_interactive.cpp` is an interactive program that allows the user to insert, find and delete key-value pairs from the hash table. The client program must be compiled and run as follows:

```bash
g++ client_interactive.cpp -o client -std=c++17 -lrt
./client
```

The client program must be run while the server program is running. The client code uses wrapper functions to interact with the shared memory object, and these wrapper functions can be found in the `inc/interface_funcs.h` header file.

There are other client programs included to test the other functionalities of the hash table, and these are in the `tests` directory. All of them can be compiled and run as follows:

```bash
g++ <test_file.cpp> -o test -std=c++17 -lrt -pthread -fsanitize=thread
./test
```

All the client programs will need the server program to be running to run successfully. The `client_multithread.cpp` is the main testing program - to demonstrate the concurrent nature of the hash table. It creates two threads that perform random operations on the hash table concurrently. 

### Alternative Approach

My initial experiments showed that while the mutex worked, it seemed to be quite a bit slower than just one thread running the operations (i.e. not needing synchronization overheads), hence not really using the concurrency feature of the hash table. I searched for alternative approaches to a mutex based concurrency model, and found that a lock-free approach using atomic variables and compare-and-swap (CAS) operations could be used. The hash tabale itself still uses a `shared_mutex` for the buckets, but the server and client programs use a lock-free queue to store and process requests. The `inc/interface_funcs_atomic.h`, `tests/client_multithread_atomic.cpp` and `server_atomic.cpp` files demonstrate this approach. This approach achieved a very large speedup compared to the mutex based approach, and was able to perform far more operations in the same time frame.

**Limitations**: Some of the atomic operations use `memory_order_relaxed` for performance, since this was mainly an experiment to see how much more performant a lock-free approach could be. On further experimentation, these relaxed memory orders lead to data races when the queue size is small and there are multiple workers. So this approach must only be used with a sufficiently large queue size. 

*Note:* All the programs were compiled using g++ version 13.3.0 on WSL with Ubuntu 24.04 and WSL2 kernel version 5.15.167.4. They have also been tested on Ubuntu 20.04 with g++ version 9.4.0 and kernel version 5.4.0-202-generic. `fsanitize` was used for checking thread safety, and is not necessary for normal compilation. The flag does not work with gcc version 9.4.0 as tested and should be removed in that case for compilation.

---