#include <iostream>
#include <string>
#include <set>
#include <map>
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
    map<string, vector<bigint>> node_to_vnode; // node, vnodes
    map<bigint, string> vnode_to_node; // vnode, node
public:
    void add_destination(string IP) {
        destinations.insert(IP);
        vector<bigint> vnodes;
        for (int i = 0; i < replication_count; i++) {
            bigint vnode = hash(IP + to_string(i));
            if (vnode_to_node.find(vnode) == vnode_to_node.end()) {
                vnodes.push_back(vnode);
                vnode_to_node[vnode] = IP;
            }
        }
        node_to_vnode[IP] = vnodes;

    }
    void remove_destination(string IP) {
        destinations.erase(IP);
        vector<bigint> vnodes = node_to_vnode[IP];
        for (bigint vnode : vnodes) {
            vnode_to_node.erase(vnode);
        }
        node_to_vnode.erase(IP);
    }
    string find_destination(string input) {
        int hash_input = hash(input);
        // other logic
        return nearest_destination;
    }
};