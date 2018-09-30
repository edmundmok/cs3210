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

void print_train_line(Stations& line) {
  for (int i=0; i<line.size(); i++) {
    if (i>0) cout << " - ";
    cout << line[i].station_num << ":" << line[i].station_name;
  }
  cout << endl;
}

void print_train_lines(Stations &green, Stations &yellow, Stations &blue) {
  cout << "green: ";
  print_train_line(green);
  cout << "yellow: ";
  print_train_line(yellow);
  cout << "blue: ";
  print_train_line(blue);
}

void print_train(Train& train) {
  cout
    << string(1, train.line)
    << train.lnum
    << " | "
    << ((train.direction == FORWARD) ? "FORWARD" : "BACKWARD")
    << " | "
    << "STATION: "
    << "l"
    << train.local_station_num
    << ":"
    << "g"
    << train.get_global_station_num()
    << ":"
    << train.stations[train.local_station_num].station_name
    << " | "
    << "START TIME: "
    << train.start_time
    << endl;
}

void print_trains(Trains& trains) {
  for (Train& train: trains) {
    print_train(train);
  }
}

void print_system_state(Trains& trains, int current_time) {
  cout << current_time << ": ";
  for (Train& train: trains) {
    if (train.start_time > current_time) continue;
    int global_station_num = train.get_global_station_num();
    cout << train.line << train.lnum << "-s" << global_station_num;
    if (train.state == MOVE)
      cout << "->s" << train.get_global_next_station_num();
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

void print_station_timings(Station& station) {
  cout << station.station_num
       << " | "
       << station.station_name
       << " | "
       << station.forward.num_waits
       << " | "
       << station.forward.total_wait_time
       << " | "
       << station.forward.min_wait_time
       << " | "
       << station.forward.max_wait_time
       << " | "
       << station.backward.num_waits
       << " | "
       << station.backward.total_wait_time
       << " | "
       << station.backward.min_wait_time
       << " | "
       << station.backward.max_wait_time
       << endl;
}

void print_stations_timings(Stations& line, string line_name, int num_trains) {
  bool has_insufficient_data = false;
  bool has_valid_wait_time = false;

  int total_maxs = 0, total_mins = 0, total_wt = 0;
  int num_maxs = 0, num_mins = 0, num_waits = 0 ;

  print_station_header(line_name);

  for (Station& station: line) {
    print_station_timings(station);

    if (station.forward.num_waits < 1 or station.backward.num_waits < 1)
      has_insufficient_data = true;
    if (station.forward.num_waits >= 1 or station.backward.num_waits >= 1)
      has_valid_wait_time = true;

    // Settle forward
    if (station.forward.num_waits > 1) {
      num_waits += station.forward.num_waits;
      total_wt += station.forward.total_wait_time;
    }

    if (station.forward.max_wait_time != NINF) {
      num_maxs++;
      total_maxs += station.forward.max_wait_time;
    }

    if (station.forward.min_wait_time != INF) {
      num_mins++;
      total_mins += station.forward.min_wait_time;
    }

    // Settle backward
    if (station.backward.num_waits > 1) {
      num_waits += station.backward.num_waits;
      total_wt += station.backward.total_wait_time;
    }

    if (station.backward.max_wait_time != NINF) {
      num_maxs++;
      total_maxs += station.backward.max_wait_time;
    }

    if (station.backward.min_wait_time != INF) {
      num_mins++;
      total_mins += station.backward.min_wait_time;
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
       << avg_wait
       << ", "
       << avg_max
       << ", "
       << avg_min
       << endl;
}

#endif //ASSIGN1_PRINT_UTILS_H
