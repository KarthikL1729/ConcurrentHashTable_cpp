#ifndef INTERFACE_FUNCS_H
#define INTERFACE_FUNCS_H

#include <iostream>
#include <atomic>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <cstring>
#include <pthread.h>

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

void perform_insert(SharedMemory* shm, int key, int value) {
    size_t current_client;
    size_t next_client;
    while (true) {
        pthread_mutex_lock(&shm->mtx);
        current_client = shm->client_id;
        next_client = (current_client + 1) % QUEUE_SIZE;

        if (next_client != shm->server_id) {
            // Space is available; proceed to enqueue
            break;
        }

        // Queue is full: release mutex and retry
        pthread_mutex_unlock(&shm->mtx);
        usleep(100); // Avoid busy-waiting
    }
    shm->client_id = next_client;
    Request& req = shm->requests[current_client];
    req.type = INSERT;
    req.key = key;
    req.value = value;
    req.processed = false;
    pthread_mutex_unlock(&shm->mtx);
    bool success;
    bool processed;
    while(true){
        pthread_mutex_lock(&shm->mtx);
        processed = req.processed;
        pthread_mutex_unlock(&shm->mtx);
        if(processed)
            break;
        usleep(100);
    }
}

void perform_read(SharedMemory* shm, int key) {
    size_t current_client;
    size_t next_client;
    while (true) {
        pthread_mutex_lock(&shm->mtx);
        current_client = shm->client_id;
        next_client = (current_client + 1) % QUEUE_SIZE;

        if (next_client != shm->server_id) {
            // Space is available; proceed to enqueue
            break;
        }

        // Queue is full: release mutex and retry
        pthread_mutex_unlock(&shm->mtx);
        usleep(100); // Avoid busy-waiting
    }
    shm->client_id = next_client;
    Request& req = shm->requests[current_client];
    req.type = READ;
    req.key = key;
    req.processed = false;
    pthread_mutex_unlock(&shm->mtx);
    bool processed;
    while(true){
        pthread_mutex_lock(&shm->mtx);
        processed = req.processed;
        pthread_mutex_unlock(&shm->mtx);
        if(processed)
            break;
        usleep(100);
    }
}

void perform_delete(SharedMemory* shm, int key) {
    size_t current_client;
    size_t next_client;
    while (true) {
        pthread_mutex_lock(&shm->mtx);
        current_client = shm->client_id;
        next_client = (current_client + 1) % QUEUE_SIZE;

        if (next_client != shm->server_id) {
            // Space is available; proceed to enqueue
            break;
        }

        // Queue is full: release mutex and retry
        pthread_mutex_unlock(&shm->mtx);
        usleep(100); // Avoid busy-waiting
    }
    shm->client_id = next_client;
    Request& req = shm->requests[current_client];
    req.type = DELETE;
    req.key = key;
    req.processed = false;
    pthread_mutex_unlock(&shm->mtx);
    bool processed;
    while(true){
        pthread_mutex_lock(&shm->mtx);
        processed = req.processed;
        pthread_mutex_unlock(&shm->mtx);
        if(processed)
            break;
        usleep(100);
    }
}

#endif