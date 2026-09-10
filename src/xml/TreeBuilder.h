#ifndef TREEBUILDER_H
#define TREEBUILDER_H

#include <string>
#include <vector>

using namespace std;

struct Node {
    int id;
    string symbol;
    vector<Node*> children;

};

class TreeBuilder{
    private: 
        int node_id;
        Node* root;
       vector<Node*> nod_stack;

    public:
        TreeBuilder();
        ~TreeBuilder();
        void createNode(string lhs, vector<string> rhs, int number_to_pop);
        void writeXML(Node* root);
};

#endif