#include "TreeBuilder.h"

TreeBuilder::TreeBuilder(): node_id(0), root(NULL)
{}

TreeBuilder::~TreeBuilder() {}

void TreeBuilder::createNode(string lhs, vector<string> rhs, int number_to_pop) {

    Node* parent = new Node();
    parent->id = node_id++;
    parent->symbol = lhs;

    if(rhs.empty()){    // rule is nullable 
        Node* nullable = new Node();
        nullable->id = node_id++;
        nullable->symbol = "ε";
        parent->children.push_back(nullable);
        return;
    }

    if(rhs.back() == "$"){  // accepted end of string
        // build root 
        // call writeXML
        return;
    }

    for(auto it = rhs.begin(); it != rhs.end(); ++it){
        Node* child = new Node();
        child->id = node_id++;
        child->symbol = *it;
        parent->children.push_back(child);
    }
}

void TreeBuilder::writeXML(Node *root) {}