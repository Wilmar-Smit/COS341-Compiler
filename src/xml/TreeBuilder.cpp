#include "TreeBuilder.h"

TreeBuilder::TreeBuilder() : node_id(0), root(NULL)
{
}

TreeBuilder::~TreeBuilder() {}

Node* TreeBuilder::createNode(string lhs, vector<Node*> rhs) {

    Node* parent = new Node();
    parent->id = node_id++;
    parent->symbol = lhs;
    parent->children = rhs;
    return parent;
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

void TreeBuilder::xmlHelper(string tag, Node* node, ofstream& file, int parent) {
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

void TreeBuilder::shiftNode(Token token) {
    string symbol = token.getCode();

    Node* leaf = new Node();
    leaf->id = node_id++;
    leaf->symbol = symbol;

    node_stack.push(leaf);
}

void TreeBuilder::reduceNode(NonTerminal nt, int number_to_pop) {
    vector<Node*> children;

    for (int i = 0; i < number_to_pop; i++) {
        children.insert(children.begin(), node_stack.top());
        node_stack.pop();
    }

    Node* parent = createNode(NonTerminalToName(nt), children);
    node_stack.push(parent);
}