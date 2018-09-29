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

#define STATION_LOCK_PREFIX "station_"
#define TRACK_LOCK_PREFIX "track_"

using namespace std;

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
    .remaining_time = get_loading_time(stations[station_idx].station_num, popularity)
  };
  return train;
}

void run_simulation(int N, train_count_t& train_count, vector<station_t>& blue_line,
                    vector<station_t>& yellow_line, vector<station_t>& green_line,
                    vector<float>& station_popularities,
                    vector<vector<int>>& dist_matrix,
                    vector<station_queue_t>& station_use,
                    vector<vector<track_queue_t>>& track_use) {

  // assign trains to thread_ids
  vector<train_t> trains(train_count.total);

  assert (green_line.size() > 0);
  assert (yellow_line.size() > 0);
  assert (blue_line.size() > 0);

  int j=0;
  // assign green line trains
  for (int i=0; i<train_count.g; i++, j++) {
    trains[j] = prepare_train(green_line, i, GREEN, station_popularities);
    if (trains[j].direction == FORWARD)
      station_use[green_line[trains[j].local_station_idx].station_num].forward_load_q.push(j);
    else
      station_use[green_line[trains[j].local_station_idx].station_num].backward_load_q.push(j);
  }

  // assign yellow line trains
  for (int i=0; i<train_count.y; i++, j++) {
    trains[j] = prepare_train(yellow_line, i, YELLOW, station_popularities);
    if (trains[j].direction == FORWARD)
      station_use[yellow_line[trains[j].local_station_idx].station_num].forward_load_q.push(j);
    else
      station_use[yellow_line[trains[j].local_station_idx].station_num].backward_load_q.push(j);
  }

  // assign blue line trains
  for (int i=0; i<train_count.b; i++, j++) {
    trains[j] = prepare_train(blue_line, i, BLUE, station_popularities);
    if (trains[j].direction == FORWARD)
      station_use[blue_line[trains[j].local_station_idx].station_num].forward_load_q.push(j);
    else
      station_use[blue_line[trains[j].local_station_idx].station_num].backward_load_q.push(j);
  }

  #pragma omp parallel num_threads(train_count.total + 1)
  {
    // master will occupy thread_id = 0, so offset all workers by 1
    int thread_id = omp_get_thread_num();
    int train_id = thread_id-1;

    for (int tick=0; tick<N; tick++) {
      // Let master print out the current state of the system
      #pragma omp master
      {
        print_system_state(trains, tick);
      }

      // Let master finish before all trains make their moves
      #pragma omp barrier

      // do some parallel work here
      if (thread_id != MASTER_THREAD) {
        // in a train
        int global_station_num = (*trains[train_id].stations)[trains[train_id].local_station_idx].station_num;
        TrainDirection direction = trains[train_id].direction;
        string direction_str = (direction == FORWARD) ? "F" : "B";

        if (trains[train_id].state == LOAD) {
          string station_lock_name = STATION_LOCK_PREFIX + direction_str + to_string(global_station_num);

          bool should_load = false;
          // check if allowed to load now
          #pragma omp critical(station_lock_name)
          {
            if (direction == FORWARD and station_use[global_station_num].forward_load_q.front() == train_id
                and station_use[global_station_num].forward_time < tick) {
              station_use[global_station_num].forward_time = tick;
              should_load = true;
            } else if (direction == BACKWARD and station_use[global_station_num].backward_load_q.front() == train_id
                       and station_use[global_station_num].backward_time < tick) {
              station_use[global_station_num].backward_time = tick;
              should_load = true;
            }
          }

          if (should_load) {
            assert(trains[train_id].remaining_time > 0);

            // check if first time loading! (door just opened!)
            if (direction == FORWARD and (*trains[train_id].stations)[trains[train_id].local_station_idx].last_forward_user != train_id) {
              (*trains[train_id].stations)[trains[train_id].local_station_idx].last_forward_user = train_id;
              // Update timings!
              if ((*trains[train_id].stations)[trains[train_id].local_station_idx].last_forward_arrival != UNDEFINED) {
                int latest_wait = tick - (*trains[train_id].stations)[trains[train_id].local_station_idx].last_forward_arrival;
                (*trains[train_id].stations)[trains[train_id].local_station_idx].total_forward_waiting_time += latest_wait;
                (*trains[train_id].stations)[trains[train_id].local_station_idx].num_forward_waits++;
                (*trains[train_id].stations)[trains[train_id].local_station_idx].min_forward_waiting_time = min((*trains[train_id].stations)[trains[train_id].local_station_idx].min_forward_waiting_time, latest_wait);
                (*trains[train_id].stations)[trains[train_id].local_station_idx].max_forward_waiting_time = max((*trains[train_id].stations)[trains[train_id].local_station_idx].max_forward_waiting_time, latest_wait);
              }
            } else if (direction == BACKWARD and (*trains[train_id].stations)[trains[train_id].local_station_idx].last_backward_user != train_id) {
              (*trains[train_id].stations)[trains[train_id].local_station_idx].last_backward_user = train_id;
              // Update timings!
              if ((*trains[train_id].stations)[trains[train_id].local_station_idx].last_backward_arrival != UNDEFINED) {
                int latest_wait = tick - (*trains[train_id].stations)[trains[train_id].local_station_idx].last_backward_arrival;
                (*trains[train_id].stations)[trains[train_id].local_station_idx].num_backward_waits++;
                (*trains[train_id].stations)[trains[train_id].local_station_idx].total_backward_waiting_time += latest_wait;
                (*trains[train_id].stations)[trains[train_id].local_station_idx].min_backward_waiting_time = min((*trains[train_id].stations)[trains[train_id].local_station_idx].min_backward_waiting_time, latest_wait);
                (*trains[train_id].stations)[trains[train_id].local_station_idx].max_backward_waiting_time = max((*trains[train_id].stations)[trains[train_id].local_station_idx].max_backward_waiting_time, latest_wait);
              }
            }

            trains[train_id].remaining_time--;
            if (trains[train_id].remaining_time == 0) {
              // doors will close after this tick!
              // update timings
              if (direction == FORWARD) {
                (*trains[train_id].stations)[trains[train_id].local_station_idx].last_forward_arrival = tick;
              } else {
                (*trains[train_id].stations)[trains[train_id].local_station_idx].last_backward_arrival = tick;
              }

              #pragma omp critical(station_lock_name)
              {
                // remove myself from the old queue since I am done!
                if (direction == FORWARD) {
                  assert(station_use[global_station_num].forward_load_q.front() == train_id);
                  (*trains[train_id].stations)[trains[train_id].local_station_idx].last_forward_user = UNDEFINED;
                  station_use[global_station_num].forward_load_q.pop();
                } else {
                  assert(station_use[global_station_num].backward_load_q.front() == train_id);
                  (*trains[train_id].stations)[trains[train_id].local_station_idx].last_backward_user = UNDEFINED;
                  station_use[global_station_num].backward_load_q.pop();
                }
              }

              if ((trains[train_id].local_station_idx == 0 and direction == BACKWARD)
                  or (trains[train_id].local_station_idx == trains[train_id].stations->size()-1 and direction == FORWARD)) {
                // check if I am at a terminal, transfer myself to the load
                // queue of the other direction of this station
                trains[train_id].direction = (trains[train_id].direction == FORWARD) ? BACKWARD : FORWARD;
                direction_str = (trains[train_id].direction == FORWARD) ? "F" : "B";
                station_lock_name = STATION_LOCK_PREFIX + direction_str + to_string(global_station_num);
                #pragma omp critical(station_lock_name)
                {
                  if (trains[train_id].direction == FORWARD) {
                    station_use[global_station_num].forward_load_q.push(train_id);
                  } else {
                    station_use[global_station_num].backward_load_q.push(train_id);
                  }
                }
                trains[train_id].remaining_time = get_loading_time(global_station_num, station_popularities);

              } else {
                // otherwise transfer myself to the track queue
                int curr_station = global_station_num;
                int next_station = (*trains[train_id].stations)[trains[train_id].local_station_idx + ((direction == FORWARD) ? 1 : -1)].station_num;
                string track_lock_name = TRACK_LOCK_PREFIX + to_string(curr_station) + string("-") + to_string(next_station);
                trains[train_id].state = MOVE;
                trains[train_id].remaining_time = dist_matrix[curr_station][next_station];

                #pragma omp critical(track_lock_name)
                {
                  track_use[curr_station][next_station].track_q.push(train_id);
                }
              }
            }
          }

        } else {
          // moving or waiting to move
          int curr_station = global_station_num;
          int next_station = (*trains[train_id].stations)[trains[train_id].local_station_idx + ((direction == FORWARD) ? 1 : -1)].station_num;
          string track_lock_name = TRACK_LOCK_PREFIX + to_string(curr_station) + string("-") + to_string(next_station);

          bool should_track = false;
          #pragma omp critical(track_lock_name)
          {
            if (track_use[curr_station][next_station].track_q.front() == train_id and track_use[curr_station][next_station].time < tick) {
              track_use[curr_station][next_station].time = tick;
              should_track = true;
            }
          }

          if (should_track) {
            assert(trains[train_id].remaining_time > 0);
            trains[train_id].remaining_time--;
            if (trains[train_id].remaining_time == 0) {
              // remove myself from the old track queue since I am done!
              #pragma omp critical(track_lock_name)
              {
                assert(track_use[curr_station][next_station].track_q.front() == train_id);
                track_use[curr_station][next_station].track_q.pop();
              }

              // I have landed at a station
              // Enqueue myself into a loading queue
              trains[train_id].remaining_time = get_loading_time(next_station, station_popularities);
              trains[train_id].state = LOAD;
              trains[train_id].local_station_idx = trains[train_id].local_station_idx + ((direction == FORWARD) ? 1 : -1);
              string station_lock_name = STATION_LOCK_PREFIX + direction_str + to_string(next_station);
              #pragma omp critical(station_lock_name)
              {
                if (direction == FORWARD) {
                  station_use[next_station].forward_load_q.push(train_id);
                } else {
                  station_use[next_station].backward_load_q.push(train_id);
                }
              }
            }
          }
        }
      }

      // Let all trains wait for master to print their state
      #pragma omp barrier
    }
  }

  // Print waiting times
  cout << "Average waiting times:" << endl;
  print_stations_timings(green_line, "green", train_count.g);
  print_stations_timings(yellow_line, "yellow", train_count.y);
  print_stations_timings(blue_line, "blue", train_count.b);
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

  // Global station lock + queue vector
  vector<station_queue_t> station_use(S);

  // Global track lock + queue vector
  vector<vector<track_queue_t>> track_use(S, vector<track_queue_t>(S));

  for (int i=0; i<S; i++){
    for (int j=0; j<S; j++) {
      cin >> dist_matrix[i][j];
    }
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
                 station_popularities, dist_matrix, station_use, track_use);

  return 0;
}