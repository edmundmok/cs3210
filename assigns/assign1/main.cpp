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

void simulate_train(train_t& train, vector<float>& station_popularities,
                    vector<vector<int>>& travel_time_matrix) {
//  if (train.travel_remaining_time > 0)
  return;
}

train_t prepare_train(vector<station_t>& stations, int i, char line,
                      vector<float>& popularity) {
  int station_idx = (i % 2 == FORWARD) ? 0 : ((int) stations.size()) - 1;
  train_t train = {
    .line = line,
    .train_num = i,
    .stations = &stations,
    .direction = (i % 2 == FORWARD) ? FORWARD : BACKWARD,
    .local_station_idx = station_idx,
    .start_time = i/2,
    .state = LOAD,
    .remaining_time = get_loading_time(i, popularity)
//    .travel_remaining_time = 0,
//    .load_remaining_time = 0
  };
  return train;
}

void run_simulation(int N, train_count_t& train_count, vector<station_t>& blue_line,
                    vector<station_t>& yellow_line, vector<station_t>& green_line,
                    vector<float>& station_popularities,
                    vector<vector<int>>& dist_matrix) {

  // assign trains to thread_ids
  vector<train_t> trains(train_count.total);

  assert (green_line.size() > 0);
  assert (yellow_line.size() > 0);
  assert (blue_line.size() > 0);

  int j=0;
  // assign green line trains
  for (int i=0; i<train_count.g; i++, j++) {
    trains[j] = prepare_train(green_line, i, GREEN, station_popularities);
    green_line[trains[j].local_station_idx].load_queue.push(j);
  }

  // assign yellow line trains
  for (int i=0; i<train_count.y; i++, j++) {
    trains[j] = prepare_train(yellow_line, i, YELLOW, station_popularities);
    yellow_line[trains[j].local_station_idx].load_queue.push(j);
  }

  // assign blue line trains
  for (int i=0; i<train_count.b; i++, j++) {
    trains[j] = prepare_train(blue_line, i, BLUE, station_popularities);
    blue_line[trains[j].local_station_idx].load_queue.push(j);
  }

  #pragma omp parallel num_threads(train_count.total + 1)
  {
    // master will occupy thread_id = 0, so offset all workers by 1
    int thread_id = omp_get_thread_num();
    int train_id = thread_id-1;

    for (int tick=0; tick<N; tick++) {
      // do some parallel work here
      if (thread_id != MASTER_THREAD) {
        // wait for all trains to make their moves this tick
        // but only if it is ready to start
        simulate_train(trains[train_id], station_popularities, dist_matrix);
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
  cout << "green: " << train_count.g << " trains -> ";
  print_stations_timings(green_line, N);
  cout << "yellow: " << train_count.y << " trains -> ";
  print_stations_timings(yellow_line, N);
  cout << "blue: " << train_count.b << " trains -> ";
  print_stations_timings(blue_line, N);
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

  // Global lock for tracks
  vector<vector<omp_lock_t>> track_lock(S, vector<omp_lock_t>(S));
  vector<vector<bool>> track_in_use(S, vector<bool>(S));

  // Global lock for station doors (1 direction per row)
  vector<vector<omp_lock_t>> door_lock(S, vector<omp_lock_t>(2));
  vector<vector<bool>> door_in_use(S, vector<bool>(2));

  for (int i=0; i<S; i++){
    for (int j=0; j<S; j++) {
      cin >> dist_matrix[i][j];
      omp_init_lock(&track_lock[i][j]);
      track_in_use[i][j] = false;
    }
    omp_init_lock(&door_lock[i][0]);
    omp_init_lock(&door_lock[i][1]);
    door_in_use[i][0] = false;
    door_in_use[i][1] = false;
  }
  cin.ignore(1, '\n');

  vector<float> station_popularities(S);
  read_popularities(cin, station_popularities);

  vector<station_t> green_line, yellow_line, blue_line;
  read_stations_for_line(cin, stations_strs, stations_map, dist_matrix, green_line);
  read_stations_for_line(cin, stations_strs, stations_map, dist_matrix, yellow_line);
  read_stations_for_line(cin, stations_strs, stations_map, dist_matrix, blue_line);

  int N = read_integer_line(cin);

  vector<string> num_trains;
  read_comma_sep_line(cin, num_trains);
  int g = stoi(num_trains[0]), y = stoi(num_trains[1]),  b = stoi(num_trains[1]);

  // Seed for reproducibility
  srand(1);

  // Include the master thread
  omp_set_num_threads(g+y+b+1);

  train_count_t train_count = {
    .g = g,
    .y = y,
    .b = b,
    .total = g+y+b
  };

  // Run simulation
  run_simulation(N, train_count, blue_line, yellow_line, green_line,
                 station_popularities, dist_matrix);

  // Destroy locks
  for (int i=0; i<S; i++) {
    for (int j=0; j<S; j++) {
      omp_destroy_lock(&track_lock[i][j]);
    }
    omp_destroy_lock(&door_lock[i][0]);
    omp_destroy_lock(&door_lock[i][1]);
  }

  return 0;
}