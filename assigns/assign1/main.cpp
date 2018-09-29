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

void run_simulation(int N, train_count_t& train_count, vector<station_t>& blue_line,
                    vector<station_t>& yellow_line, vector<station_t>& green_line,
                    vector<float>& station_popularities,
                    vector<vector<int>>& dist_matrix,
                    vector<station_queue_t>& station_use,
                    vector<vector<track_queue_t>>& track_use) {

  // assign trains to thread_ids
  vector<Train> trains;

  assert (green_line.size() > 0);
  assert (yellow_line.size() > 0);
  assert (blue_line.size() > 0);

  // assign green line trains
  int j=0;
  for (int i=0; i<train_count.g; i++, j++) {
    trains.push_back(Train(GREEN, i, j, green_line, station_popularities,
                           station_use, track_use));
    trains[j].queue_for_station_use();
  }

  // assign yellow line trains
  for (int i=0; i<train_count.y; i++, j++) {
    trains.push_back(Train(YELLOW, i, j, yellow_line, station_popularities,
                           station_use, track_use));
    trains[j].queue_for_station_use();
  }

  // assign blue line trains
  for (int i=0; i<train_count.b; i++, j++) {
    trains.push_back(Train(BLUE, i, j, blue_line, station_popularities,
                           station_use, track_use));
    trains[j].queue_for_station_use();
  }

  assert(trains.size() == train_count.total);

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
        Train& train = trains[train_id];
        assert(train.gnum == train_id);

        // in a train
        int global_station_num = train.get_global_station_num();
        TrainDirection direction = train.direction;

        if (train.state == LOAD) {
          bool should_load = false;
          // check if allowed to load now
          #pragma omp critical(train.get_station_lock_name())
          {
            if (train.should_load(tick)) {
              should_load = true;
              train.acknowledge_load(tick);
            }
          }

          if (should_load) {
            assert(train.remaining_time > 0);

            // check if first time loading! (door just opened!)
            if (train.has_door_just_opened()) {
              train.update_station_wait_times_as_arrival(tick);
            }

            train.update_remaining_time();

            if (train.remaining_time == 0) {
              // update timings
              train.update_station_wait_times_as_departure(tick);

              #pragma omp critical(station_lock_name)
              {
                // remove myself from the old queue since I am done!
                train.remove_as_station_user();
                train.dequeue_from_station_use();
              }

              if (train.is_at_terminal_station()) {
                // next move should be other direction load
                train.reverse_train_direction();
                #pragma omp critical(train.get_station_lock_name())
                {
                  train.queue_for_station_use();
                }
                train.reset_remaining_time();
              } else {
                // next move should be wait for track
                train.state = MOVE;

                #pragma omp critical(train.get_track_lock_name)
                {

                }

              }

//              } else {
//                // otherwise transfer myself to the track queue
//                int curr_station = global_station_num;
//                int next_station = (*trains[train_id].stations)[trains[train_id].local_station_idx + ((direction == FORWARD) ? 1 : -1)].station_num;
//                string track_lock_name = TRACK_LOCK_PREFIX + to_string(curr_station) + string("-") + to_string(next_station);
//                trains[train_id].state = MOVE;
//                trains[train_id].remaining_time = dist_matrix[curr_station][next_station];
//
//                #pragma omp critical(track_lock_name)
//                {
//                  track_use[curr_station][next_station].track_q.push(train_id);
//                }
//              }
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