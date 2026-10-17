#ifndef CODEGENERATOR_H
#define CODEGENERATOR_H

#include <iostream>
#include "symbolTable.h"
#include "TreeBuilder.h"
#include "typeChecker.h"

using namespace std;

class CodeGenerator {
private:
    SymbolTable symbol_table;
    int scope_id;
    
    int variable_counter;
    int label_counter;

    int newVariable();
    int newLable();
    string semicolon();

public:
    CodeGenerator(SymbolTable st);
    string transAlgo(Node* code);
    string transInstr(Node* code);
    string transOutp(Node* code);
    string transCall(Node* code);
    string transLoop(Node* code);
    string transAssign(Node* code);
    string transBranch(Node* code);
    string transTerm(Node* code, string result_va);
    string transCond(Node* code);
    string tansBool(Node* code);

};
#endif