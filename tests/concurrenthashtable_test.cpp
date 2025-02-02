#include "../inc/ChainedConcurrentHT.h"

using namespace std;

int main() {
    ConcurrentHashTable<int, int> hash_table(100);
    // Insert values
    hash_table.insert(1, 35426);
    hash_table.insert(2, 124342);
    hash_table.insert(101, 2345);
    hash_table.insert(201, 14325);
    hash_table.insert(1, 2747);

    // Retrieve values
    hash_table.find(1);

    hash_table.find(101);

    // Remove a key
    hash_table.remove(2);
    hash_table.remove(101);

    // Should not be found
    hash_table.find(101);
    
    return 0;
}