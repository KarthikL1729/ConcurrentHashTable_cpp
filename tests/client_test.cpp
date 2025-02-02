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
    perform_insert(shared_mem, 5, 10);   
    perform_read(shared_mem, 5);         
    perform_delete(shared_mem, 5);       
    perform_read(shared_mem, 5);   
    perform_insert(shared_mem, 5, 102534);
    perform_read(shared_mem, 5);
    perform_insert(shared_mem, 6, 34);     
    perform_insert(shared_mem, 7, 234);
    perform_insert(shared_mem, 8, 44);
    perform_insert(shared_mem, 108, 24);
    perform_insert(shared_mem, 208, 224);
    perform_insert(shared_mem, 16736, 214);
    perform_delete(shared_mem, 108);

    // Signal server to shutdown
    shared_mem->done = true;
    // Cleanup
    munmap(shared_mem, SHM_SIZE);
    close(shm_fd);

    return 0;
}