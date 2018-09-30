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

void run_simulation(int max_tick, TrainCounts& train_counts,
                    Stations& blue_line, Stations& yellow_line,
                    Stations& green_line, Popularities& station_popularities,
                    AdjMatrix& dist_matrix, StationUses& station_use,
                    TrackUses& track_use) {

  // assign trains to thread_ids
  Trains trains;

  assert (green_line.size() > 0);
  assert (yellow_line.size() > 0);
  assert (blue_line.size() > 0);

  // assign green line trains
  int j=0;
  for (int i=0; i<train_counts.num_greens; i++, j++) {
    trains.push_back(Train(GREEN, i, j, green_line, dist_matrix, station_popularities,
                           station_use, track_use));
    trains[j].queue_for_station_use();
  }

  // assign yellow line trains
  for (int i=0; i<train_counts.num_yellows; i++, j++) {
    trains.push_back(Train(YELLOW, i, j, yellow_line, dist_matrix, station_popularities,
                           station_use, track_use));
    trains[j].queue_for_station_use();
  }

  // assign blue line trains
  for (int i=0; i<train_counts.num_blues; i++, j++) {
    trains.push_back(Train(BLUE, i, j, blue_line, dist_matrix, station_popularities,
                           station_use, track_use));
    trains[j].queue_for_station_use();
  }

  assert(trains.size() == train_counts.num_total);

  #pragma omp parallel num_threads(train_counts.num_total + 1)
  {
    // master will occupy thread_id = 0, so offset all workers by 1
    int thread_id = omp_get_thread_num();
    int train_id = thread_id-1;

    for (int tick=0; tick<max_tick; tick++) {
      // Let master print out the current state of the system
      #pragma omp master
      {
//        print_trains(trains);
        print_system_state(trains, tick);
      }

      // Let master finish before all trains make their moves
      #pragma omp barrier

      // do some parallel work here
      if (thread_id != MASTER_THREAD) {
        Train& train = trains[train_id];
        assert(train.gnum == train_id);

        if (train.state == LOAD) {
          bool should_load = false;
          // check if allowed to load now
          string station_lock_name = train.get_station_lock_name();
          #pragma omp critical(station_lock_name)
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
                station_lock_name = train.get_station_lock_name();
                #pragma omp critical(station_lock_name)
                {
                  train.queue_for_station_use();
                }
                train.reset_remaining_time_for_load();
              } else {
                // next move should be wait for track
                train.state = MOVE;
                string track_lock_name = train.get_track_lock_name();
                #pragma omp critical(track_lock_name)
                {
                  train.queue_for_track_use();
                }
                train.reset_remaining_time_for_track();
              }
            }
          }

        } else {
          // moving or waiting to move
          bool should_move_on_track = false;
          string track_lock_name = train.get_track_lock_name();
          #pragma omp critical(track_lock_name)
          {
            if (train.should_move_on_track(tick)) {
              train.acknowledge_move_on_track(tick);
              should_move_on_track = true;
            }
          }

          if (should_move_on_track) {
            assert(train.remaining_time > 0);
            train.update_remaining_time();
            if (train.remaining_time == 0) {
              #pragma omp critical(track_lock_name)
              {
                train.dequeue_from_track_use();
              }

              // Enqueue into a loading queue
              train.progress_to_load_at_next_station();
              string station_lock_name = train.get_station_lock_name();
              #pragma omp critical(station_lock_name)
              {
                train.queue_for_station_use();
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
  print_stations_timings(green_line, "green", train_counts.num_greens);
  print_stations_timings(yellow_line, "yellow", train_counts.num_yellows);
  print_stations_timings(blue_line, "blue", train_counts.num_blues);
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
  AdjMatrix dist_matrix(S, vector<int>(S));

  for (int i=0; i<S; i++){
    for (int j=0; j<S; j++) {
      cin >> dist_matrix[i][j];
    }
  }
  cin.ignore(1, '\n');

  // Global station lock + queue vector
  StationUses station_use(S);

  // Global track lock + queue vector
  TrackUses track_use(S, vector<TrackUse>(S));

  // Read station popularities
  Popularities station_popularities(S);
  read_popularities(cin, station_popularities);

  Stations green_line, yellow_line, blue_line;
  read_stations_for_line(cin, stations_strs, stations_map, dist_matrix, green_line);
  read_stations_for_line(cin, stations_strs, stations_map, dist_matrix, yellow_line);
  read_stations_for_line(cin, stations_strs, stations_map, dist_matrix, blue_line);

  int N = read_integer_line(cin);

  vector<string> num_trains;
  read_comma_sep_line(cin, num_trains);
  int num_greens = stoi(num_trains[0]),
    num_yellows = stoi(num_trains[1]),
    num_blues = stoi(num_trains[2]);

  TrainCounts train_counts(num_greens, num_yellows, num_blues);

  // Include the master thread (+1 from total trains)
  omp_set_num_threads(train_counts.num_total + 1);

  // Run simulation
  run_simulation(N, train_counts, blue_line, yellow_line, green_line,
                 station_popularities, dist_matrix, station_use, track_use);

  return 0;
}