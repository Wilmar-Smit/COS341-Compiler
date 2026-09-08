#ifndef FILE_READER
#define FILE_READER

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

std::string readFileToString(const std::string &filePath) {
  std::ifstream file(filePath);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file: " + filePath);
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string content = buffer.str();

  // Replace newlines and carriage returns with spaces
  for (char &ch : content) {
    if (ch == '\n' || ch == '\r') {
      ch = ' ';
    }
  }

  // Remove double/multiple spaces
  std::string result;
  result.reserve(content.size());
  bool inSpace = false;

  for (char ch : content) {
    if (ch == ' ') {
      if (!inSpace) {
        result.push_back(ch);
        inSpace = true;
      }
    } else {
      result.push_back(ch);
      inSpace = false;
    }
  }

  return result+"$";
}

#endif
