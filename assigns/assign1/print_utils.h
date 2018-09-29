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

void print_system_state(vector<Train>& trains, int current_time) {
  cout << current_time << ": ";
  for (Train& train: trains) {
    if (train.start_time > current_time) continue;
    int global_station_num = train.get_global_station_num();
    cout << train.line << train.lnum << "-s" << global_station_num;
    if (train.state == MOVE)
      cout << "->s" << (train.direction == FORWARD) ? global_station_num + 1 : global_station_num - 1;
    cout << ", ";
  }
  cout << endl;
}

void print_station_header(string& line_name) {
  cout << "DEBUG " << line_name << endl;
  cout << "station_num"
       << " | "
       << "station_name"
       << " | "
       << "num_forward_waits"
       << " | "
       << "total_forward_waiting_time"
       << " | "
       << "min_forward_waiting_time"
       << " | "
       << "max_forward_waiting_time"
       << " | "
       << "num_backward_waits"
       << " | "
       << "total_backward_waiting_time"
       << " | "
       << "min_backward_waiting_time"
       << " | "
       << "max_backward_waiting_time"
       << endl;
}

void print_station_timings(station_t& station) {
  cout << station.station_num
       << " | "
       << station.station_name
       << " | "
       << station.num_forward_waits
       << " | "
       << station.total_forward_waiting_time
       << " | "
       << station.min_forward_waiting_time
       << " | "
       << station.max_forward_waiting_time
       << " | "
       << station.num_backward_waits
       << " | "
       << station.total_backward_waiting_time
       << " | "
       << station.min_backward_waiting_time
       << " | "
       << station.max_backward_waiting_time
       << endl;
}

void print_stations_timings(vector<station_t>& line, string line_name, int num_trains) {
  bool has_insufficient_data = false;
  bool has_valid_wait_time = false;

  int total_maxs = 0, total_mins = 0, total_wt = 0;
  int num_maxs = 0, num_mins = 0, num_waits = 0 ;

//  print_station_header(line_name);

  for (station_t& station: line) {
//    print_station_timings(station);

    if (station.num_forward_waits < 1 or station.num_backward_waits < 1)
      has_insufficient_data = true;
    if (station.num_forward_waits >= 1 or station.num_forward_waits >= 1)
      has_valid_wait_time = true;

    // Settle forward
    if (station.num_forward_waits > 1) {
      num_waits += station.num_forward_waits;
      total_wt += station.total_forward_waiting_time;
    }

    if (station.max_forward_waiting_time != NINF) {
      num_maxs++;
      total_maxs += station.max_forward_waiting_time;
    }

    if (station.min_forward_waiting_time != INF) {
      num_mins++;
      total_mins += station.min_forward_waiting_time;
    }

    // Settle backward
    if (station.num_backward_waits > 1) {
      num_waits += station.num_backward_waits;
      total_wt += station.total_backward_waiting_time;
    }

    if (station.max_backward_waiting_time != NINF) {
      num_maxs++;
      total_maxs += station.max_backward_waiting_time;
    }

    if (station.min_backward_waiting_time != INF) {
      num_mins++;
      total_mins += station.min_backward_waiting_time;
    }
  }
  float avg_wait = total_wt / float(num_waits);
  float avg_max = total_maxs / float(num_maxs);
  float avg_min = total_mins / float(num_mins);

  int num_stations = (int) line.size();
  if (!has_valid_wait_time) {
    cout
      << "The " << line_name
      << " line does not have >= 1 station with at least two arrivals."
      << " Since there is no proper wait time for at least one stations, "
      << "the statistics cannot be computed properly." << endl;
    return;
  }
  if (has_insufficient_data)
    cout
      << "(The "
      << line_name
      << " line has >= 1 station(s) without at least two arrivals,"
      << " so these stations do not have a proper waiting time!"
      << " The waiting times reported is only for those stations on this line that do.)"
      << endl;
  cout << line_name
       << ": "
       << num_trains
       << " trains -> "
       << setprecision(2)
       << avg_wait
       << ", "
       << avg_max
       << ", "
       << avg_min
       << endl;
}

#endif //ASSIGN1_PRINT_UTILS_H
