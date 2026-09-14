#include "TreeBuilder.h"

TreeBuilder::TreeBuilder() : node_id(0)
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

    int depth = 0;

    file << "<tree>\n";
    depth++;

    file << indent(depth) << "<root>\n";
    depth++;

    file << indent(depth) << "<id>" << root->id << "</id>\n";
    file << indent(depth) << "<content>" << root->symbol << "</content>\n";
    file << indent(depth) << "<children>\n";
    depth++;
    for (auto i = root->children.begin(); i != root->children.end(); ++i) {
        file << indent(depth) << "<child>" << (*i)->id << "</child>\n";

    }
    depth--;
    file << indent(depth) << "</children>\n";
    depth--;
    file << indent(depth) << "</root>\n";

    for (auto i = root->children.begin(); i != root->children.end(); ++i) {
        xmlHelper("node", *i, file, root->id,depth);
    }

    depth--;
    file << indent(depth) << "</tree>\n";
    file.close();
}

void TreeBuilder::xmlHelper(string tag, Node* node, ofstream& file, int parent, int depth) {
    file << indent(depth) << "<" << tag << ">\n";
    depth++;

    file << indent(depth) << "<id>" << node->id << "</id>\n";
    file << indent(depth) << "<content>" << node->symbol << "</content>\n";
    file << indent(depth) << "<parent>" << parent << "</parent>\n";

    if (!node->children.empty()) { // inner
        file << indent(depth) << "<children>\n";
        depth++;
        for (auto i = node->children.begin(); i != node->children.end(); ++i) {
            file << indent(depth) << "<child>" << (*i)->id << "</child>\n";
        }
        depth--;
        file << indent(depth) << "</children>\n";
    }

    depth--;
    file << indent(depth) << "</" << tag << ">\n";

    for (auto i = node->children.begin(); i != node->children.end(); ++i) {
        xmlHelper("node", *i, file, node->id, depth);
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

Node* TreeBuilder::getRoot()const {
    return this->node_stack.top();
}

string TreeBuilder::indent(int depth) {
    return string(depth, '\t');
}