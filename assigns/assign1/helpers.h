//
// Created by Edmund Mok on 9/28/18.
//

#ifndef ASSIGN1_HELPERS_H
#define ASSIGN1_HELPERS_H

#include <vector>
#include <unordered_set>
#include "network.h"

using namespace std;

bool is_terminal_station(vector<vector<int>>& M, int station_idx,
                         unordered_set<int>& stations) {
  int count = 0;
  for (int i=0; i<M.size(); i++) {
    if (stations.find(i) != stations.end() && M[station_idx][i] > 0) {
      count++;
    }
    if (count > 1) return false;
  }
  return true;
}

int get_neighbour(vector<vector<int>>& M, int curr,
                  unordered_set<int>& stations, unordered_set<int>& visited) {
  for (int i=0; i<M.size(); i++) {
    if ((visited.find(i) == visited.end())
        && (stations.find(i) != stations.end())
        && (M[curr][i])) {
      return i;
    }
  }
  return -1;
}

void line_up_stations(vector<vector<int>>& M, vector<string>& stations_strs,
                      unordered_set<int>& stations,
                      vector<station_t>& lined_stations) {
  // first find the starting point
  int curr = -1;
  for (int station_idx: stations) {
    if (is_terminal_station(M, station_idx, stations)) {
      curr = station_idx;
      break;
    }
  }

  station_t first_station = {};
  first_station.station_num = curr;
  first_station.station_name = stations_strs[curr];

  lined_stations.push_back(first_station);
  unordered_set<int> visited = {curr};

  // go down the line from the starting point
  while (true) {
    curr = get_neighbour(M, curr, stations, visited);
    if (curr == -1) return;

    station_t curr_station = {};
    curr_station.station_num = curr;
    curr_station.station_name = stations_strs[curr];
    visited.insert(curr);
    lined_stations.push_back(curr_station);
  }
}

#endif //ASSIGN1_HELPERS_H
