
#include "CodeGenerator.h"

int CodeGenerator::newVariable() {
    ++this->variable_counter;
    return variable_counter;
}

int CodeGenerator::newLable() {
    ++this->label_counter;
    return label_counter;
}

string CodeGenerator::semicolon() {
    return ";\n";
}

CodeGenerator::CodeGenerator(SymbolTable st)
    : symbol_table(st), variable_counter(0), label_counter(0) {
}

string CodeGenerator::transAlgo(Node* code) {
    if (code->children.size() == 0) {   // ε -  nullable 
        return "/* end */";
    }

    return transInstr(code->children[0]) +// INSTR
        semicolon() +
        transAlgo(code->children[2]);
}

string CodeGenerator::transInstr(Node* code) {
    Node* first = code->children[0];

    if (first->symbol == "print") return transOutp(code->children[1]);

    if (first->symbol == "nop") return "/* nop */";

    if (first->symbol == "comment") return "/*" + code->children[1]->symbol + "*/";

    if (first->symbol == "ASSIGN") return transAssign(first);

    if (first->symbol == "BRANCH") return transBranch(first);

    if (first->symbol == "LOOP") return transLoop(first);
}

string CodeGenerator::transOutp(Node* code) {
    Node* first = code->children[0];

    if (first->symbol == "(") {
        int var_num = newVariable();
        string var;
        string variable = transTerm(code->children[1],var);
        

        // TODO: NEED TO FINISH THIS

        return "";
    }

    return "std::cout<<\"" + first->symbol + "\"";

}

// PART 3b
string CodeGenerator::transCall(Node* code) {}

string CodeGenerator::transLoop(Node* code) {}

string CodeGenerator::transAssign(Node* code) {
}

string CodeGenerator::transBranch(Node* code) {}

string CodeGenerator::transTerm(Node* code, string result_var) {
    string name = code->children[0]->symbol;
    VariableSymbol var_sym;
   // symbol_table.lookupVariable(name, )
}

string CodeGenerator::transCond(Node* code) {}

string CodeGenerator::tansBool(Node* code) {}