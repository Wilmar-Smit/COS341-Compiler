#include "TreeBuilder.h"

TreeBuilder::TreeBuilder() : node_id(0), root(NULL)
{
}

TreeBuilder::~TreeBuilder() {}

void TreeBuilder::createNode(string lhs, vector<string> rhs, int number_to_pop) {

    Node* parent = new Node();
    parent->id = node_id++;
    parent->symbol = lhs;

    if (rhs.empty()) {    // rule is nullable 
        Node* nullable = new Node();
        nullable->id = node_id++;
        nullable->symbol = "ε";
        parent->children.push_back(nullable);
        return;
    }

    if (rhs.back() == "$") {  // accepted end of string
        // build root 
        // call writeXML
        return;
    }

    for (auto it = rhs.begin(); it != rhs.end(); ++it) {
        Node* child = new Node();
        child->id = node_id++;
        child->symbol = *it;
        parent->children.push_back(child);
    }
}

void TreeBuilder::writeXML(Node* root) {
    // build root 
    ofstream file("tree.xml");

    file << "<tree>\n";
    file << "<root>\n";
    file << "<id>" << root->id << "</id>\n";
    file << "<content>" << root->symbol << "</content>\n";
    file << "<children>\n";
    for (auto i = root->children.begin(); i != root->children.end(); ++i) {
        file << "<child>" << (*i)->id << "</child>\n";
    }
    file << "</children>\n";
    file << "</root>\n";

    for (auto i = root->children.begin(); i != root->children.end(); ++i) {
        xmlHelper("node", *i, file, root->id);
    }

    file << "</tree>\n";
    file.close();
}

void xmlHelper(string tag, Node* node, ofstream& file, int parent) {
    file << "<" << tag << ">\n";
    file << "<id>" << node->id << "</id>\n";
    file << "<content>" << node->symbol << "</content>\n";
    file << "<parent>" << parent << "</parent>\n";

    if (!node->children.empty()) { // inner
        file << "<children>\n";

        for (auto i = node->children.begin(); i != node->children.end(); ++i) {
            file << "<child>" << (*i)->id << "</child>\n";
        }

        file << "</children>\n";
    }

    file << "</" << tag << ">\n";

    for (auto i = node->children.begin(); i != node->children.end(); ++i) {
        xmlHelper("node", *i, file, node->id);
    }
}