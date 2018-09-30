#ifndef ASSIGN1_READ_UTILS_H
#define ASSIGN1_READ_UTILS_H

#include <string>
#include <iostream>
#include <sstream>
#include <vector>

#include "network.h"
#include "helpers.h"

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

void read_stations(istream& is, unordered_map<string, int>& stations_map,
                   unordered_set<int>& station_set) {
  vector<string> stations;
  read_comma_sep_line(is, stations);
  for (string& station: stations) {
    station_set.insert(stations_map[station]);
  }
}

void read_popularities(istream& is, Popularities& popularities) {
  vector<string> popularity_strs;
  read_comma_sep_line(cin, popularity_strs);
  for (int i=0; i<popularity_strs.size(); i++) {
    popularities[i] = stof(popularity_strs[i]);
  }
}

void read_stations_for_line(istream& is,
                            vector<string>& stations_str,
                            unordered_map<string, int>& stations_map,
                            vector<vector<int>>& dist_matrix,
                            Stations& line) {
  unordered_set<int> stations;
  read_stations(is, stations_map, stations);
  line_up_stations(dist_matrix, stations_str, stations, line);
}

#endif //ASSIGN1_READ_UTILS_H
