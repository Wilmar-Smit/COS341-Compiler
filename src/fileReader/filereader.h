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
  buffer << file.rdbuf(); // Reads the entire file buffer into the stream
  return buffer.str();
}

#endif
