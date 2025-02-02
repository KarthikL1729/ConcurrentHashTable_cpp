#include "../inc/interface_funcs_atomic.h"
#include <thread>

int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
int t1_inserts = 0;
int t2_inserts = 0;
int t1_reads = 0;
int t2_reads = 0;
int t1_deletes = 0;
int t2_deletes = 0;
SharedMemory* shared_mem = (SharedMemory*)mmap(
    nullptr, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0
);

void t1_func() {
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
            t1_inserts++;
        }
        else if(operation == 1)
        {
            perform_read(shared_mem, key);
            t1_reads++;
        }
        else
        {
            perform_delete(shared_mem, key);
            t1_deletes++;
        }
    }
}

void t2_func() {
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
            t2_inserts++;
        }
        else if(operation == 1)
        {
            perform_read(shared_mem, key);
            t2_reads++;
        }
        else
        {
            perform_delete(shared_mem, key);
            t2_deletes++;
        }
    }
}

int main() {
    srand(time(NULL));
    thread t1(t1_func);
    thread t2(t2_func);

    t1.join();
    t2.join();

    cout << "T1 inserts: " << t1_inserts << endl;
    cout << "T1 reads: " << t1_reads << endl;
    cout << "T1 deletes: " << t1_deletes << endl;
    cout << "T2 inserts: " << t2_inserts << endl;
    cout << "T2 reads: " << t2_reads << endl;
    cout << "T2 deletes: " << t2_deletes << endl;
    shared_mem->done.store(true);
    
    munmap(shared_mem, SHM_SIZE);
    close(shm_fd);
    return 0;
}