//
// Created by Edmund Mok on 9/28/18.
//

#ifndef ASSIGN1_PRINT_UTILS_H
#define ASSIGN1_PRINT_UTILS_H

#include <iostream>
#include <vector>
#include "network.h"

using namespace std;

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
         // print track here if necessary
         << ", ";
  }
  cout << endl;
}

void print_stations_timings(vector<station_t>& line, int total_time) {
  int total_waiting_time = 0, total_longest_time = 0, total_shortest_time = 0;
  for (station_t& station: line) {
    total_waiting_time += station.total_waiting_time;
    total_longest_time += station.max_waiting_time;
    total_shortest_time += station.min_waiting_time;
  }
  int num_stations = (int) line.size();
  cout << setprecision(2)
       << 0.0
       //       << ((float) total_waiting_time) / (num_stations * )
       << ", "
       << ((float) total_longest_time) / num_stations
       << ", "
       << ((float) total_shortest_time) / num_stations
       << endl;
}

#endif //ASSIGN1_PRINT_UTILS_H
