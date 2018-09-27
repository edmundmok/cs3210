#include <iostream>
#include <omp.h>
#include <unordered_map>
#include <unordered_set>
#include <cstdlib>
#include <vector>
#include <sstream>

#define INF 999999999
#define NINF -999999999

#define GREEN 'G'
#define BLUE 'B'
#define YELLOW 'Y'

#define FORWARD 0
#define BACKWARD 1

using namespace std;

int N;

struct train_count_t {
  int g;
  int y;
  int b;
  int total;
};

struct train_t {
//  string& train_id;
  char line;
  int train_num;
  vector<int> *stations;
  int direction;
  int station_idx;
  int start_time;
};

struct network_t {
  train_count_t *train_count;
  vector<int> *blue_line;
  vector<int> *yellow_line;
  vector<int> *green_line;
};

struct station_t {
  int last_arrival = 0;
  int total_waiting_time = 0;
  int num_arrivals = 0;
  int min_waiting_time = INF;
  int max_waiting_time = NINF;
} station_t;

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

void print_vector(vector<int>& v) {
  for (int e: v) {
    cout << e << " ";
  }
  cout << endl;
}

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

void line_up_stations(vector<vector<int>>& M,  unordered_set<int>& stations,
                      vector<int>& lined_stations) {
  // first find the starting point
  int curr = -1;
  for (int station_idx: stations) {
    if (is_terminal_station(M, station_idx, stations)) {
      curr = station_idx;
      break;
    }
  }

  lined_stations.push_back(curr);
  unordered_set<int> visited = {curr};

  // go down the line from the starting point
  while (true) {
    curr = get_neighbour(M, curr, stations, visited);
    if (curr == -1) return;
    visited.insert(curr);
    lined_stations.push_back(curr);
  }
}

int get_loading_time(int i, vector<float>& popularity) {
  return popularity[i] * ((rand() % 10) + 1);
}

void simulate_train() {
//  int thread_id = omp_get_thread_num();
  return;
}

void add_trains(int T, int g, int y, int b) {
  return;
}

void run_simulation(int S, network_t *network) {

  // assign trains to thread_ids
  train_t trains[network->train_count->total];

  int j=0;
  // assign green line trains
  for (int i=0; i<network->train_count->g; i++, j++) {
    trains[j] = {
      .line = GREEN,
      .train_num = i,
      .stations = network->green_line,
      .direction = (i % 2 == FORWARD) ? FORWARD : BACKWARD,
      .station_idx = (i % 2 == FORWARD) ? 0 : network->train_count->g - 1,
      .start_time = i/2
    };
  }

  // assign yellow line trains
  for (int i=0; i<network->train_count->y; i++, j++) {

  }

  // assign blue line trains
  for (int i=0; i<network->train_count->b; i++, j++) {

  }


  #pragma omp parallel num_threads(network->train_count->total)
  {
    for (int t=0; t<N; t++) {
      // t is the current tick

      // simulate_train();

      // wait for all trains to make their moves this tick
      #pragma omp barrier
    }
  }
}

int main() {
  int S = read_integer_line(cin);

  // Map station name to index number
  unordered_map<string, int> stations_map;
  get_station_map(cin, stations_map);

  // Setup M
  vector<vector<int>> M(S, vector<int>(S));
  vector<omp_lock_t> station_door_lock(S);
  vector<vector<omp_lock_t>> M_lock(S, vector<omp_lock_t>(S));
  for (int i=0; i<S; i++){
    for (int j=0; j<S; j++) {
      cin >> M[i][j];
      omp_init_lock(&M_lock[i][j]);
    }
    omp_init_lock(&station_door_lock[i]);
  }
  cin.ignore(1, '\n');

  vector<float> popularity(S);
  vector<string> popularity_strs;
  read_comma_sep_line(cin, popularity_strs);
  for (int i=0; i<S; i++) {
    popularity[i] = stof(popularity_strs[i]);
  }

  unordered_set<int> green, yellow, blue;
  read_stations(cin, stations_map, green);
  read_stations(cin, stations_map, yellow);
  read_stations(cin, stations_map, blue);

  vector<int> green_line, yellow_line, blue_line;
  line_up_stations(M, green, green_line);
  line_up_stations(M, yellow, yellow_line);
  line_up_stations(M, blue, blue_line);

  cout << "green: ";
  print_vector(green_line);
  cout << "yellow: ";
  print_vector(yellow_line);
  cout << "blue: ";
  print_vector(blue_line);

  N = read_integer_line(cin);

  vector<string> num_trains;
  read_comma_sep_line(cin, num_trains);
  int g = stoi(num_trains[0]),
    y = stoi(num_trains[1]),
    b = stoi(num_trains[1]);

  // Seed for reproducibility
  srand(1);

  // Assuming this excludes the master thread
  omp_set_num_threads(g+y+b);

//  run_simulation();

  for (int i=0; i<S; i++) {
    for (int j=0; j<S; j++) {
      omp_destroy_lock(&M_lock[i][j]);
    }
    omp_destroy_lock(&station_door_lock[i]);
  }

  return 0;
}