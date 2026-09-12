#ifndef CSVREADER_H
#define CSVREADER_H

#include "../Parser/ParserActions/parser.actions.h"
#include <string>
#include <fstream>
#include <sstream>
#include <ios>
#include <vector>

using namespace std;

// Template method: read() owns the shared file/line/word parsing loop and
// defers building each cell to create(), which subclasses implement.
class TableReader {
public:
    virtual ~TableReader() {}

    vector<vector<ParserAction *>> read(const string &filePath) {
        fstream fin;
        fin.open(filePath, ios::in);

        string temp, line, word;
        vector<vector<ParserAction *>> table;
        vector<ParserAction *> row;

        while (fin >> temp) {
            row.clear();

            getline(fin, line);
            stringstream s(line);

            while (getline(s, word, ',')) {
                row.push_back(create(word));
            }

            table.push_back(row);
        }

        return table;
    }

protected:
    virtual ParserAction *create(const string &word) = 0;
};

class ActionTableReader : public TableReader {
protected:
    ParserAction *create(const string &word) override {
        
    }
};

class GotoTableReader : public TableReader {
protected:
    ParserAction *create(const string &word) override {

    }
};

#endif