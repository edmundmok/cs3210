#ifndef ASSIGN1_HELPERS_H
#define ASSIGN1_HELPERS_H

#include <vector>
#include <unordered_set>
#include <unordered_map>

using namespace std;

void get_station_map(unordered_map<string, int>& station_map,
                     vector<string>& stations) {
  for (int i=0; i<stations.size(); i++) station_map[stations[i]] = i;
}

#endif //ASSIGN1_HELPERS_H
