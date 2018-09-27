#include <iostream>
#include <omp.h>
#include <unordered_map>
#include <unordered_set>
#include <cstdlib>
#include <vector>
#include <string>
#include <sstream>
#include <cassert>

#define INF 999999999
#define NINF -999999999

#define GREEN 'g'
#define BLUE 'b'
#define YELLOW 'y'

#define FORWARD 0
#define BACKWARD 1

#define MASTER_THREAD 0

using namespace std;

int N;

struct train_count_t {
  int g;
  int y;
  int b;
  int total;
};

struct station_t {
  int station_num;
  string station_name;
  int last_arrival = 0;
  int num_arrivals = 0;
  int total_waiting_time = 0;
  int min_waiting_time = INF;
  int max_waiting_time = NINF;
};

struct train_t {
  char line;
  int train_num;
  vector<station_t> *stations;
  int direction;
  int local_station_idx;
  int start_time;
};

struct network_t {
  train_count_t *train_count;
  vector<station_t> *blue_line;
  vector<station_t> *yellow_line;
  vector<station_t> *green_line;
};

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

void get_station_map(istream& is, unordered_map<string, int>& station_map,
                     vector<string>& stations) {
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

void print_train_line(vector<station_t>& line) {
  for (int i=0; i<line.size(); i++) {
    if (i>0) cout << " - ";
    cout << line[i].station_num << ":" << line[i].station_name;
  }
  cout << endl;
}

void print_train_lines(vector<station_t> &green, vector<station_t> &yellow,
                       vector<station_t> &blue) {
  cout << "green: ";
  print_train_line(green);
  cout << "yellow: ";
  print_train_line(yellow);
  cout << "blue: ";
  print_train_line(blue);
}

void print_train(train_t& train) {
  cout
    << string(1, train.line)
    << train.train_num
    << " | "
    << ((train.direction == FORWARD) ? "FORWARD" : "BACKWARD")
    << " | "
    << "STATION: "
    << "l"
    << train.local_station_idx
    << ":"
    << "g"
    << (*train.stations)[train.local_station_idx].station_num
    << ":"
    << (*train.stations)[train.local_station_idx].station_name
    << " | "
    << "START TIME: "
    << train.start_time
    << endl;
}

void print_trains(vector<train_t>& trains) {
  for (train_t& train: trains) {
    print_train(train);
  }
}

void print_system_state(vector<train_t>& trains, int current_time) {
  cout << current_time << ": ";
  for (train_t& train: trains) {
    if (train.start_time > current_time) continue;
    cout << train.line << train.train_num
      << "-s"
      << (*train.stations)[train.local_station_idx].station_num
      << ", ";
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

int get_loading_time(int i, vector<float>& popularity) {
  return popularity[i] * ((rand() % 10) + 1);
}

void simulate_train(int train_id) {

  return;
}

void run_simulation(network_t *network) {

  // assign trains to thread_ids
  vector<train_t> trains(network->train_count->total);

  assert ((*network->green_line).size() > 0);
  assert ((*network->yellow_line).size() > 0);
  assert ((*network->blue_line).size() > 0);

  int j=0;
  // assign green line trains
  for (int i=0; i<network->train_count->g; i++, j++) {
    train_t train = {
      .line = GREEN,
      .train_num = i,
      .stations = network->green_line,
      .direction = (i % 2 == FORWARD) ? FORWARD : BACKWARD,
      .local_station_idx = (i % 2 == FORWARD) ? 0 : ((int) (*network->green_line).size()) - 1,
      .start_time = i/2
    };
    trains[j] = train;
  }

  // assign yellow line trains
  for (int i=0; i<network->train_count->y; i++, j++) {
    train_t train = {
      .line = YELLOW,
      .train_num = i,
      .stations = network->yellow_line,
      .direction = (i % 2 == FORWARD) ? FORWARD : BACKWARD,
      .local_station_idx = (i % 2 == FORWARD) ? 0 : ((int) (*network->yellow_line).size()) - 1,
      .start_time = i/2
    };
    trains[j] = train;
  }

  // assign blue line trains
  for (int i=0; i<network->train_count->b; i++, j++) {
    train_t train = {
      .line = BLUE,
      .train_num = i,
      .stations = network->blue_line,
      .direction = (i % 2 == FORWARD) ? FORWARD : BACKWARD,
      .local_station_idx = (i % 2 == FORWARD) ? 0 : ((int) (*network->blue_line).size()) - 1,
      .start_time = i/2
    };
    trains[j] = train;
  }

  #pragma omp parallel num_threads(network->train_count->total + 1)
  {
    // master will occupy thread_id = 0, so offset all workers by 1
    int thread_id = omp_get_thread_num();
    int train_id = thread_id-1;

    for (int tick=0; tick<N; tick++) {
      // do some parallel work here
      if (thread_id != MASTER_THREAD) {
        // wait for all trains to make their moves this tick
        // but only if it is ready to start
        if (trains[train_id].start_time <= tick) simulate_train(train_id);
      }

      // Let master wait for all trains to make their moves
      #pragma omp barrier

      // Let master print out the current state of the system
      #pragma omp master
      {
        print_system_state(trains, tick);
      }

      // Let all trains wait for master to print their state
      #pragma omp barrier
    }


  }
}

int main() {
  int S = read_integer_line(cin);

  // Map station name to index number
  unordered_map<string, int> stations_map;
  vector<string> stations_strs;
  read_comma_sep_line(cin, stations_strs);
  get_station_map(cin, stations_map, stations_strs);

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

  vector<station_t> green_line, yellow_line, blue_line;
  line_up_stations(M, stations_strs, green, green_line);
  line_up_stations(M, stations_strs, yellow, yellow_line);
  line_up_stations(M, stations_strs, blue, blue_line);

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

  train_count_t train_count = {
    .g = g,
    .y = y,
    .b = b,
    .total = g+y+b
  };

  network_t network = {
    .train_count = &train_count,
    .blue_line = &blue_line,
    .yellow_line = &yellow_line,
    .green_line = &green_line,
  };

  run_simulation(&network);

  for (int i=0; i<S; i++) {
    for (int j=0; j<S; j++) {
      omp_destroy_lock(&M_lock[i][j]);
    }
    omp_destroy_lock(&station_door_lock[i]);
  }

  return 0;
}