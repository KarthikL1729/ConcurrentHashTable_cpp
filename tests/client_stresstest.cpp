#include "../inc/interface_funcs.h"

int main() {
    // Open shared memory
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        cerr << "Client: shm_open failed" << endl;
        return 1;
    }

    // Map shared memory
    SharedMemory* shared_mem = (SharedMemory*)mmap(
        nullptr, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0
    );
    if (shared_mem == MAP_FAILED) {
        cerr << "Client: mmap failed" << endl;
        close(shm_fd);
        return 1;
    }
    
    // loop for 10 seconds
    time_t start = time(NULL);
    while(time(NULL) - start < 60)
    {
        //randomly choose an operation
        int operation = rand() % 3;
        int key = rand() % 1000;
        int value = rand() % 1000;
        if(operation == 0)
        {
            perform_insert(shared_mem, key, value);
        }
        else if(operation == 1)
        {
            perform_read(shared_mem, key);
        }
        else
        {
            perform_delete(shared_mem, key);
        }
    }

    // Signal server to shutdown
    shared_mem->done = true;
    // Cleanup
    munmap(shared_mem, SHM_SIZE);
    close(shm_fd);

    return 0;
}