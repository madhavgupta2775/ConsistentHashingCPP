#include <iostream>
#include <string>
#include <set>
#include "SHA256.h"
#include "bigint.h"

using namespace std;

struct ring
{
private:
    int ring_node_count;
    int supported_node_count;
    int replication_count;
    bigint hash(string input) {
        SHA256 sha;
        sha.update(input);
        string hash = sha.toString(sha.digest());
        return bigint(hash);
    }
    set<string> destinations;
public:
    void add_destination(string IP) {
        destinations.insert(IP);
        // other logic

    }
    void remove_destination(string IP) {
        destinations.erase(IP);
        // other logic
    }
    string find_destination(string input) {
        int hash_input = hash(input);
        // other logic
        return nearest_destination;
    }
};