#ifndef TREEBUILDER_H
#define TREEBUILDER_H

#include <string>
#include <vector>
#include <fstream>
#include <stack>
#include <optional>
#include <map>
#include <iostream>

#include "token.h"
#include "ProductionRules.h"
using namespace std;


inline string NonTerminalToName(NonTerminal nt) {
    static const map<NonTerminal, string> table = {
        { NonTerminal::SPL_PROG, "SPL_PROG" },{ NonTerminal::P, "P" },
        { NonTerminal::V_DECL, "V_DECL" },{ NonTerminal::F_DECL, "F_DECL" },
        { NonTerminal::F_TYPE, "F_TYPE" },{ NonTerminal::ALGO, "ALGO" },
        { NonTerminal::OUTP, "OUTP" },{ NonTerminal::INSTR, "INSTR" },
        { NonTerminal::CALL, "CALL" },{ NonTerminal::INPUT, "INPUT" },
        { NonTerminal::ASSIGN, "ASSIGN" },{ NonTerminal::TERM, "TERM" },
        { NonTerminal::BRANCH, "BRANCH" },{ NonTerminal::BOOL, "BOOL" },
        { NonTerminal::LOOP, "LOOP" },{ NonTerminal::COND, "COND" },
    };

    return table.at(nt);
}

struct Node {
    int id;
    string symbol;
    vector<Node*> children;

};

class TreeBuilder {
private:
    int node_id;
    stack<Node*> node_stack;

    void xmlHelper(string tag, Node* node, ofstream& file, int parent, int depth);
    string indent(int depth);
public:
    TreeBuilder();
    ~TreeBuilder();
    Node* createNode(string lhs, vector<Node*> rhs);
    void writeXML(Node* root);
    void shiftNode(Token token);
    void reduceNode(NonTerminal nt, int number_to_pop);
    Node* getRoot()const;
};


#endif