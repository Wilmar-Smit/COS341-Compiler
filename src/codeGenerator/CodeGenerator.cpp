
#include "CodeGenerator.h"

string CodeGenerator::newVariable() {
    return "v" + variable_counter++;
}

string CodeGenerator::newLabel() {
    return "label_" + label_counter++;
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
        string variable = newVariable();
        string translation = transTerm(code->children[1], variable);
        string type = (symbol_table.getNumberMode() == NumberMode::INTEGER) ? "int" : "float";

        return type + " " + variable + ";\n" +
            translation + ";\n" + "std::cout<<" + variable;
    }

    return "std::cout<<\"" + first->symbol + "\"";

}


string CodeGenerator::transLoop(Node* code) {
    Node* first = code->children[0];
    stringstream ss;
    string start = newLabel();
    string end = newLabel();
    string do_label = newLabel();
    string decision;

    if (first->symbol == "COND") {
        string body = transAlgo(code->children[4]);

        if (first->children[0]->symbol == "until") {
            decision = transBool(code->children[1], end, do_label);
        }

        if (first->children[0]->symbol == "while") {
            decision = transBool(code->children[1], do_label, end);
        }

        ss << start << ": /* until */\n";
        ss << decision << ";\n";
        ss << do_label << ": /* do */\n";
        ss << body << ";\n";
        ss << "goto " << start << " ;\n";
        ss << end << " :\n";
        ss << "/* exit */\n";

        return ss.str();
    }

    if (first->symbol == "do") {
        string body = transAlgo(code->children[2]);
        Node* cond = code->children[4];

        if (cond->children[0]->symbol == "until") {
            decision = transBool(code->children[1], do_label, end);
        }

        if (cond->children[0]->symbol == "while") {
            decision = transBool(code->children[1], end, do_label);
        }

        ss << do_label << ": /* do */\n";
        ss << body << ";\n";
        ss << "/* while */\n";
        ss << decision << ";\n";
        ss << end << ": /* exit */\n";

        return ss.str();
    }
}

string CodeGenerator::transAssign(Node* code) {
    string variable = newVariable();
    string translation = transTerm(code->children[2], variable);
    string intern = code->children[0]->symbol;
    string type = (symbol_table.getNumberMode() == NumberMode::INTEGER) ? "int" : "float";


    return type + " " + variable + ";\n" +
        translation + ";\n" +
        intern + " = " + variable;

}

string CodeGenerator::transBranch(Node* code) {
    string label_true = newLabel();
    string label_false = newLabel();
    string label_1 = newLabel();

    string decision = transBool(code->children[1], label_true, label_false);
    string true_code = transAlgo(code->children[4]);
    string false_code = transAlgo(code->children[7]);

    stringstream ss;

    ss << "/*if*/\n";
    ss << decision << ";\n";
    ss << label_false << ": /*else*/ \n";
    ss << false_code << ";\n";
    ss << "goto " << label_1 << ";\n";
    ss << label_true << ": /*then*/\n";
    ss << true_code << ";";
    ss << label_1 << ": /*continue*/\n";

    return ss.str();
}

string CodeGenerator::transTerm(Node* code, string result_var) {
    string first = code->children[0]->symbol;

    string v1 = newVariable();
    string v2 = newVariable();

    if (first == "mod") {
        return transTerm(code->children[2], v1) + transTerm(code->children[3], v2) + "[ " + result_var + " = " + v1 + " % " + v2;
    }

    if (first == "div") {
        return transTerm(code->children[2], v1) + transTerm(code->children[3], v2) + "[ " + result_var + " = " + v1 + " / " + v2;
    }

    if (first == "add") {
        return transTerm(code->children[2], v1) + transTerm(code->children[3], v2) + "[ " + result_var + " = " + v1 + " + " + v2;
    }

    if (first == "sub") {
        return transTerm(code->children[2], v1) + transTerm(code->children[3], v2) + "[ " + result_var + " = " + v1 + " - " + v2;
    }

    if (first == "mul") {
        return transTerm(code->children[2], v1) + transTerm(code->children[3], v2) + "[ " + result_var + " = " + v1 + " * " + v2;
    }

    if (first == "neg") {
        return transTerm(code->children[2], v1) + "[" + result_var + " = - " + v1 + "]";
    }

    return result_var + " = " + first;
}

string CodeGenerator::transBool(Node* code, string label_true, string label_false) {
    Node* first = code->children[0];
    stringstream ss;

    if (first->symbol == "not") {
        return transBool(code, label_false, label_true);
    }

    if (first->symbol == "or") {
        ss << transBool(code->children[2], label_true, label_false);
        ss << ";\n";
        ss << transBool(code->children[3], label_true, label_false);

        return ss.str();
    }

    if (first->symbol == "and") {
        ss << transBool(code->children[2], label_false, label_true) << ";\n";
        ss << transBool(code->children[3], label_true, label_false);
    }

    string x = newVariable();
    string y = newVariable();

    string code1 = transTerm(code->children[2], x);
    string code2 = transTerm(code->children[3], y);
    string op;

    if (first->symbol == "eq") {
        op = "==";
    }
    else if (first->symbol == "larger") {
        op = ">";
    }
    else if (first->symbol == "lesser") {
        op = "<";
    }

    ss << code1 << ";\n";
    ss << code2 << ";\n";
    ss << "if ( " << x << " " << op << " " << y << " ) goto " << label_true;

    return ss.str();
}

void CodeGenerator::generateCode(Node* root) {
    ofstream file("output.cpp");
    file << transAlgo(root);
    file.close();
}