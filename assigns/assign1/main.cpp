#include <iostream>
#include <omp.h>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <sstream>


using namespace std;

void read_stations(unordered_map<string, int>& stations, unordered_set<int>& station_set) {
  string stations_str;
  string station;
  getline(cin, stations_str);
  stringstream stations_sstream(stations_str);

  while (getline(stations_sstream, station)) {
    station_set.insert(stations[station]);
  }
}

int main() {
  int S;
  cin >> S;

  // Remove extra newline after reading S
  string station;
  getline(cin, station);

  // Map station name to index number
  unordered_map<string, int> stations;
  string stations_str;
  getline(cin, stations_str);
  stringstream stations_sstream(stations_str);
  for (int i=0; i<S; i++) {
    getline(stations_sstream, station, ',');
    stations[station] = i;
  }

  // Setup M
  int M[S][S];
  for (int i=0; i<S; i++){
    for (int j=0; j<S; j++) {
      cin >> M[i][j];
    }
  }

  getline(cin, station);

  // Setup P
  float P[S];
  string popularity;
  string pop_str;
  getline(cin, pop_str);
  stringstream pop_sstream(pop_str);
  for (int i=0; i<S; i++) {
    getline(pop_sstream, popularity, ',');
    P[i] = stof(popularity);
  }

  unordered_set<int> green, yellow, blue;

  read_stations(stations, green);
  read_stations(stations, yellow);
  read_stations(stations, blue);

  int N;
  cin >> N;


  string num_stations_str;
  getline(cin, num_stations_str);
  getline(cin, num_stations_str);
  stringstream num_stations_sstream(num_stations_str);

  int g, y, b;
  string num_station;
  getline(num_stations_sstream, num_station, ',');
  g = stof(num_station);
  getline(num_stations_sstream, num_station, ',');
  y = stof(num_station);
  getline(num_stations_sstream, num_station, ',');
  b = stof(num_station);
  
  return 0;
}