#include <iostream>
#include <omp.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <sstream>


using namespace std;

int N, T;

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

void get_station_map(istream& is, unordered_map<string, int>& station_map) {
  vector<string> stations;
  read_comma_sep_line(cin, stations);
  for (int i=0; i<stations.size(); i++) station_map[stations[i]] = i;
}

void read_stations(istream& is, unordered_map<string, int>& stations_map,
                   unordered_set<int>& station_set) {
  vector<string> stations;
  read_comma_sep_line(is, stations);
  for (string& station: stations) {
    station_set.insert(stations_map[station]);
  }
}

void simulate_train() {

  return;
}

void add_trains() {
  return;
}

void get_positions() {
  return;
}

void print_positions() {
  return;
}

void run_simulation() {
  for (T = 0; T < N; T++) {
    add_trains();
  }

//  while (1) {
//    add_trains();
//    get_positions();
//    print_positions();
//  }
}

int main() {
  int S = read_integer_line(cin);

  // Map station name to index number
  unordered_map<string, int> stations_map;
  get_station_map(cin, stations_map);

  // Setup M
  int M[S][S];
  omp_lock_t station_lock[S];
  omp_lock_t M_lock[S][S];
  for (int i=0; i<S; i++){
    for (int j=0; j<S; j++) {
      cin >> M[i][j];
      omp_init_lock(&M_lock[i][j]);
    }
    omp_init_lock(&station_lock[i]);
  }
  cin.ignore(1, '\n');

  float P[S];
  vector<string> popularities;
  read_comma_sep_line(cin, popularities);
  for (int i=0; i<S; i++) {
    P[i] = stof(popularities[i]);
  }

  unordered_set<int> green, yellow, blue;
  read_stations(cin, stations_map, green);
  read_stations(cin, stations_map, yellow);
  read_stations(cin, stations_map, blue);

  N = read_integer_line(cin);

  vector<string> num_trains;
  read_comma_sep_line(cin, num_trains);
  int g = stoi(num_trains[0]),
    y = stoi(num_trains[1]),
    b = stoi(num_trains[1]);

  // Seed for reproducibility
  srand(1);

//  run_simulation();

  for (int i=0; i<S; i++) {
    for (int j=0; j<S; j++) {
      omp_destroy_lock(&M_lock[i][j]);
    }
    omp_destroy_lock(&station_lock[i]);
  }

  return 0;
}