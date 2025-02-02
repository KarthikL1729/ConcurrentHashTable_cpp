#ifndef CHAINED_CONCURRENT_HT_H
#define CHAINED_CONCURRENT_HT_H

#include <iostream>
#include <vector>
#include <list>
#include <mutex>
#include <shared_mutex>

using namespace std;

template <typename Key, typename Value> class ConcurrentHashTable {
    private:
        // Node for linked list
        struct Node {
            Key key;
            Value value;
            Node(const Key k, const Value v) : key(k), value(v) {}
        };
        // Bucket for chaining, each is guarded by a mutex
        struct Bucket {
            list<Node> chain;
            shared_mutex mutex; // Mutex per bucket for fine-grained locking
        };

        int Hash(const Key key, int num_buckets) {
            return key % num_buckets;
        }

        // Main data structure
        vector<Bucket> buckets;

        // Get the bucket for a given key

        Bucket& get_bucket(const Key key) {
            size_t index = Hash(key, buckets.size());
            return buckets[index];
        }

    public:
        ConcurrentHashTable(size_t num_buckets) : buckets(num_buckets) {}

        void insert(const Key key, const Value value) {
            auto& bucket = get_bucket(key);
            unique_lock lock(bucket.mutex);
            cout << "Write operation..."<<endl;
            // Check if key already exists in the chain
            for (auto& node : bucket.chain) {
                if (node.key == key) {
                    cout << "Key: "<<key<<endl;
                    cout << "Key already exists" << endl;
                    cout << "Updating value" << endl;
                    node.value = value; // Update existing key
                    cout <<"Updated key: "<<key<<endl;
                    cout <<"Updated value: "<<value<<endl;
                    cout <<"Bucket index: "<<Hash(key, buckets.size())<<endl;
                    cout <<"Current size of bucket: "<<bucket.chain.size()<<endl;
                    return;
                }
            }
            // Key not found; add new node to the chain
            bucket.chain.emplace_back(key, value);
            cout <<"Inserted key: "<<key<<endl;
            cout <<"Inserted value: "<<value<<endl;
            cout <<"Bucket index: "<<Hash(key, buckets.size())<<endl;
            cout <<"Current size of bucket: "<<bucket.chain.size()<<endl;
        }

        bool find(const Key key) {
            auto& bucket = get_bucket(key);
            shared_lock lock(bucket.mutex);
            cout << "Read operation..."<<endl;

            for (const auto& node : bucket.chain) {
                if (node.key == key) {
                    cout << "Key: "<<key<<endl;
                    cout << "Value: "<<node.value<<endl;
                    cout << "Bucket index: "<<Hash(key, buckets.size())<<endl;
                    cout << "Current size of bucket: "<<bucket.chain.size()<<endl;
                    return true;
                }
            }
            cout << "Key "<<key<< " not found."<<endl;
            return false;
        }

        bool remove(const Key key) {
            auto& bucket = get_bucket(key);
            unique_lock lock(bucket.mutex);
            cout << "Delete operation..."<<endl;

            auto it = bucket.chain.begin();
            while (it != bucket.chain.end()) {
                if (it->key == key) {
                    bucket.chain.erase(it);
                    cout << "Key "<<key<< " deleted."<<endl;
                    cout << "Bucket index: "<<Hash(key, buckets.size())<<endl;
                    cout << "Current size of bucket: "<<bucket.chain.size()<<endl;
                    return true;
                }
                ++it;
            }
            cout << "Key "<<key<< "not found."<<endl;
            return false;
        }

        void print() {
            for (int i = 0; i < buckets.size(); i++) {
                cout << "Bucket " << i << ": ";
                for (const auto& node : buckets[i].chain) {
                    cout << "(" << node.key << ", " << node.value << ") ";
                }
                cout << endl;
            }
        }
};

#endif