//
// Created by Edmund Mok on 9/28/18.
//

#ifndef ASSIGN1_NETWORK_H
#define ASSIGN1_NETWORK_H

#include <vector>
#include <queue>

#define UNDEFINED_WAIT -99

#define INF 999999999
#define NINF -999999999

#define FORWARD 0
#define BACKWARD 1

using namespace std;

struct train_count_t;
struct station_t;
struct train_t;

struct train_count_t {
  int g;
  int y;
  int b;
  int total;
};

struct station_t {
  int station_num;
  string station_name;
  queue<train_t *> load_queue;
  int last_arrival = UNDEFINED_WAIT;
  int num_arrivals = 0;
  int total_waiting_time = UNDEFINED_WAIT;
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
  int travel_remaining_time;  // If 0, means currently at a station.
  int load_remaining_time;  // If 0, means doors are closed, and can move if track is ready.
};

struct network_t {
  train_count_t *train_count;
  vector<station_t> *blue_line;
  vector<station_t> *yellow_line;
  vector<station_t> *green_line;
  vector<float> *station_popularities;
  vector<vector<int>> *travel_time_matrix;
};

#endif //ASSIGN1_NETWORK_H
