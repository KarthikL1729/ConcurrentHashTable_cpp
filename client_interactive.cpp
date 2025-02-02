#include "inc/interface_funcs.h"

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

    // Accept inputs in a while loop

    while(!shared_mem->done) {
        // Accept inputs
        cout << "Enter operation (insert, read, delete): ";
        string operation;
        cin >> operation;

        if(operation == "insert") {
            cout << "Enter key: ";
            int key;
            cin >> key;
            cout << "Enter value: ";
            int value;
            cin >> value;
            perform_insert(shared_mem, key, value);
        } else if(operation == "read") {
            cout << "Enter key: ";
            int key;
            cin >> key;
            perform_read(shared_mem, key);
        } else if(operation == "delete") {
            cout << "Enter key: ";
            int key;
            cin >> key;
            perform_delete(shared_mem, key);
        } else {
            cout << "Invalid operation" << endl;
        }
        if(operation == "done")
            shared_mem->done = true;
    }

    return 0;
}