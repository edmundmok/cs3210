//
// Created by Edmund Mok on 9/28/18.
//

#ifndef ASSIGN1_NETWORK_H
#define ASSIGN1_NETWORK_H

#include <vector>
#include <queue>
#include <omp.h>

#define UNDEFINED -99

#define INF 999999999
#define NINF -999999999

#define UNKNOWN_TRAIN -1

using namespace std;

struct train_count_t;
struct station_t;
struct train_t;

enum TrainState { LOAD, MOVE };

enum TrainDirection { FORWARD, BACKWARD };

struct train_count_t {
  int g;
  int y;
  int b;
  int total;
};

struct station_queue_t {
  // Last use times
  int forward_time = -1;
  int backward_time = -1;

  // Load queues
  queue<int> forward_load_q;
  queue<int> backward_load_q;
};

struct track_queue_t {
  // Last use time
  int time = -1;

  // Track queues
  queue<int> track_q;
};

struct station_t {
  int station_num;
  string station_name;

  // Forward stats
  // Last user
  int last_forward_user = UNKNOWN_TRAIN;
  int last_forward_arrival = UNDEFINED;
  int num_forward_waits = 0;
  int total_forward_waiting_time = 0;
  int min_forward_waiting_time = INF;
  int max_forward_waiting_time = NINF;

  // Backward stats
  int last_backward_user = UNKNOWN_TRAIN;
  int last_backward_arrival = UNDEFINED;
  int num_backward_waits = 0;
  int total_backward_waiting_time = 0;
  int min_backward_waiting_time = INF;
  int max_backward_waiting_time = NINF;
};

int get_loading_time(int i, vector<float>& popularity) {
  // use the formula and take ceiling because train must stop for at least
  // that amount but ticks are integers
  return ceil(popularity[i] * ((rand() % 10) + 1));
}

class Train {
public:
  char line;
  int num; // train number for this particular line
  vector<station_t>& stations;
  TrainDirection direction;
  TrainState state;
  int local_station_num;
  int remaining_time;

  int start_time; // For printing and debugging purposes

  Train(char line, int num, vector<station_t>& stations, int local_station_num,
  vector<float>& popularities) :
  line(line), num(num), stations(stations), state(LOAD) {
    int num_stations = stations.size();
    this->direction = (num % 2) ? FORWARD : BACKWARD;
    this->local_station_num = (num % 2) ? 0 : num_stations-1;
    int global_station_num = stations[this->local_station_num].station_num;
    this->remaining_time = get_loading_time(global_station_num, popularities);
    this->start_time = num/2;
  };
};

struct train_t {
  char line;
  int train_num;
  vector<station_t> *stations;
  TrainDirection direction;
  int local_station_idx;
  int start_time;
  TrainState state;
  int remaining_time;
};

#endif //ASSIGN1_NETWORK_H
