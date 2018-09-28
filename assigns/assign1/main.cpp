#include <iostream>
#include <omp.h>
#include <unordered_map>
#include <unordered_set>
#include <cstdlib>
#include <iomanip>
#include <vector>
#include <math.h>
#include <string>
#include <queue>
#include <sstream>
#include <cassert>

#include "read_utils.h"
#include "network.h"
#include "print_utils.h"
#include "helpers.h"

#define GREEN 'g'
#define BLUE 'b'
#define YELLOW 'y'


#define MASTER_THREAD 0

using namespace std;

int get_loading_time(int i, vector<float>& popularity) {
  // use the formula and take ceiling because train must stop for at least
  // that amount but ticks are integers
  return ceil(popularity[i] * ((rand() % 10) + 1));
}

void simulate_train(train_t& train, vector<float> *station_popularities,
                    vector<vector<int>> *travel_time_matrix) {
//  if (train.travel_remaining_time > 0)
  return;
}

train_t prepare_train(vector<station_t> *stations, int i, char line) {
  int station_idx = (i % 2 == FORWARD) ? 0 : ((int) (*stations).size()) - 1;
  train_t train = {
    .line = line,
    .train_num = i,
    .stations = stations,
    .direction = (i % 2 == FORWARD) ? FORWARD : BACKWARD,
    .local_station_idx = station_idx,
    .start_time = i/2,
    .travel_remaining_time = 0,
    .load_remaining_time = 0
  };
  return train;
}

void run_simulation(int N, network_t *network) {

  // assign trains to thread_ids
  vector<train_t> trains(network->train_count->total);

  assert ((*network->green_line).size() > 0);
  assert ((*network->yellow_line).size() > 0);
  assert ((*network->blue_line).size() > 0);

  int j=0;
  // assign green line trains
  for (int i=0; i<network->train_count->g; i++, j++) {
    trains[j] = prepare_train(network->green_line, i, GREEN);
    (*network->green_line)[trains[j].local_station_idx].load_queue.push(&trains[j]);
  }

  // assign yellow line trains
  for (int i=0; i<network->train_count->y; i++, j++) {
    trains[j] = prepare_train(network->yellow_line, i, YELLOW);
    (*network->yellow_line)[trains[j].local_station_idx].load_queue.push(&trains[j]);
  }

  // assign blue line trains
  for (int i=0; i<network->train_count->b; i++, j++) {
    trains[j] = prepare_train(network->blue_line, i, BLUE);
    (*network->blue_line)[trains[j].local_station_idx].load_queue.push(&trains[j]);
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
        if (trains[train_id].start_time <= tick)
          simulate_train(trains[train_id], network->station_popularities,
                         network->travel_time_matrix);
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

  // Print waiting times
  cout << "Average waiting times:" << endl;
  cout << "green: " << network->train_count->g << " trains -> ";
  print_stations_timings(*network->green_line, N);
  cout << "yellow: " << network->train_count->y << " trains -> ";
  print_stations_timings(*network->yellow_line, N);
  cout << "blue: " << network->train_count->b << " trains -> ";
  print_stations_timings(*network->blue_line, N);
}

int main() {
  // Read number of train stations in network
  int S = read_integer_line(cin);

  // Read list of stations and
  vector<string> stations_strs;
  read_comma_sep_line(cin, stations_strs);
  // map station name to index number
  unordered_map<string, int> stations_map;
  get_station_map(cin, stations_map, stations_strs);

  // Read adjacency matrix of network and
  // setup dist_matrix
  vector<vector<int>> dist_matrix(S, vector<int>(S));
  vector<omp_lock_t> door_lock(S);
  vector<bool> door_in_use(S);

  vector<vector<omp_lock_t>> track_lock(S, vector<omp_lock_t>(S));
  vector<vector<bool>> track_in_use(S, vector<bool>(S));
  for (int i=0; i<S; i++){
    for (int j=0; j<S; j++) {
      cin >> dist_matrix[i][j];
      omp_init_lock(&track_lock[i][j]);
      track_in_use[i][j] = false;
    }
    omp_init_lock(&door_lock[i]);
    door_in_use[i] = false;
  }
  cin.ignore(1, '\n');

  vector<float> station_popularities(S);
  read_popularities(cin, station_popularities);

  unordered_set<int> green, yellow, blue;
  read_stations(cin, stations_map, green);
  read_stations(cin, stations_map, yellow);
  read_stations(cin, stations_map, blue);

  vector<station_t> green_line, yellow_line, blue_line;
  line_up_stations(dist_matrix, stations_strs, green, green_line);
  line_up_stations(dist_matrix, stations_strs, yellow, yellow_line);
  line_up_stations(dist_matrix, stations_strs, blue, blue_line);

  int N = read_integer_line(cin);

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
    .station_popularities = &station_popularities
  };

  run_simulation(N, &network);

  for (int i=0; i<S; i++) {
    for (int j=0; j<S; j++) {
      omp_destroy_lock(&track_lock[i][j]);
    }
    omp_destroy_lock(&door_lock[i]);
  }

  return 0;
}