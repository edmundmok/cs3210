#ifndef ASSIGN1_READ_UTILS_H
#define ASSIGN1_READ_UTILS_H

#include <string>
#include <iostream>
#include <sstream>
#include <vector>

using namespace std;

int read_integer_line(istream& is) {
  int n;
  is >> n;
  is.ignore(1, '\n');  // remove newline at the end
  return n;
}

void read_comma_sep_line(istream& is, vector<string>& sep_strs) {
  string line, temp;
  getline(is, line);
  stringstream line_stream(line);
  while (getline(line_stream, temp, ',')) {
    sep_strs.push_back(temp);
  }
}

#endif //ASSIGN1_READ_UTILS_H
