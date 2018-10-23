//
// Created by Edmund Mok on 9/28/18.
//

#ifndef ASSIGN1_HELPERS_H
#define ASSIGN1_HELPERS_H

#include <vector>
#include <unordered_set>
#include "network.h"

using namespace std;

void get_station_map(istream& is, unordered_map<string, int>& station_map,
                     vector<string>& stations) {
  for (int i=0; i<stations.size(); i++) station_map[stations[i]] = i;
}

#endif //ASSIGN1_HELPERS_H
