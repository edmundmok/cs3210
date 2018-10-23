#ifndef ASSIGN1_HELPERS_H
#define ASSIGN1_HELPERS_H

#include <vector>
#include <unordered_set>
#include <unordered_map>

using namespace std;

bool is_terminal_station_for_line(AdjMatrix &dist_matrix, int station_idx,
                                  unordered_set<int> &stations) {
  int count = 0;
  for (int i=0; i<dist_matrix.size(); i++) {
    bool has_station = stations.find(i) != stations.end(),
      has_track = dist_matrix[station_idx][i] > 0;
    if (has_station and has_track) {
      count++;
    }
    if (count > 1) return false;
  }
  return true;
}

int get_neighbour(AdjMatrix& dist_matrix, int curr,
                  unordered_set<int>& stations, unordered_set<int>& visited) {
  for (int i=0; i<dist_matrix.size(); i++) {
    bool has_visited = visited.find(i) != visited.end(),
      has_station = stations.find(i) != stations.end(),
      has_track = dist_matrix[curr][i] > 0;
    if ((not has_visited) and has_station and has_track) {
      return i;
    }
  }
  return -1;
}

void line_up_stations(AdjMatrix& dist_matrix, vector<string>& stations_strs,
                      unordered_set<int>& stations, vector<int>& lined_stations) {
  // first find the starting point
  int curr = -1;
  for (int station_idx: stations) {
    if (is_terminal_station_for_line(dist_matrix, station_idx, stations)) {
      curr = station_idx;
      break;
    }
  }

  int first_station = curr;
  lined_stations.push_back(first_station);
  unordered_set<int> visited = { curr };

  // go down the line from the starting point
  while (true) {
    curr = get_neighbour(dist_matrix, curr, stations, visited);
    if (curr == -1) return;
    visited.insert(curr);
    lined_stations.push_back(curr);
  }
}

void get_station_map(unordered_map<string, int>& station_map,
                     vector<string>& stations) {
  for (int i=0; i<stations.size(); i++) station_map[stations[i]] = i;
}

#endif //ASSIGN1_HELPERS_H
