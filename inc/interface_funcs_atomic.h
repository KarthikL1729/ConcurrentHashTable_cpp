#ifndef INTERFACE_FUNCS_H
#define INTERFACE_FUNCS_H

#include <iostream>
#include <atomic>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <cstring>

using namespace std;

#define SHM_NAME "shm_concurrent_hash_table"
#define SHM_SIZE (sizeof(SharedMemory))
#define QUEUE_SIZE 1024

enum RequestType {
    INSERT,
    READ,
    DELETE
};

struct Request {
    atomic<RequestType> type;
    atomic<int> key;
    atomic<int> value;
    atomic<bool> processed;
    atomic<bool> success;
};

struct SharedMemory {
    atomic<bool> done;
    Request requests[QUEUE_SIZE];
    atomic<size_t> client_id;
    atomic<size_t> server_id;
};

void perform_insert(SharedMemory* shm, int key, int value) {
    size_t current_client;
    size_t next_client;
    do {
        current_client = shm->client_id.load(memory_order_relaxed);
        next_client = (current_client + 1) % QUEUE_SIZE;
    } while (!shm->client_id.compare_exchange_weak(
        current_client, next_client,
        memory_order_release, memory_order_relaxed));

    Request& req = shm->requests[current_client];
    req.type.store(INSERT, memory_order_relaxed);
    req.key.store(key, memory_order_relaxed);
    req.value.store(value, memory_order_relaxed);
    req.processed.store(false, memory_order_relaxed);

    while (!req.processed.load(memory_order_acquire)) {}
}

void perform_read(SharedMemory* shm, int key) {
    size_t current_client;
    size_t next_client;
    do {
        current_client = shm->client_id.load(memory_order_relaxed);
        next_client = (current_client + 1) % QUEUE_SIZE;
    } while (!shm->client_id.compare_exchange_weak(
        current_client, next_client,
        memory_order_release, memory_order_relaxed));

    Request& req = shm->requests[current_client];
    req.type.store(READ, memory_order_relaxed);
    req.key.store(key, memory_order_relaxed);
    req.processed.store(false, memory_order_relaxed);

    while (!req.processed.load(memory_order_acquire)) {}
}

void perform_delete(SharedMemory* shm, int key) {
    size_t current_client;
    size_t next_client;
    do {
        current_client = shm->client_id.load(memory_order_relaxed);
        next_client = (current_client + 1) % QUEUE_SIZE;
    } while (!shm->client_id.compare_exchange_weak(
        current_client, next_client,
        memory_order_release, memory_order_relaxed));

    Request& req = shm->requests[current_client];
    req.type.store(DELETE, memory_order_relaxed);
    req.key.store(key, memory_order_relaxed);
    req.processed.store(false, memory_order_relaxed);

    while (!req.processed.load(memory_order_acquire)) {}
}

#endif