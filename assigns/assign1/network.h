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

  // Queue locks
  omp_lock_t forward_load_lock;
  omp_lock_t backward_load_lock;
};

struct track_queue_t {
  // Last use times
  int time = -1;

  // Track queues
  queue<int> track_q;

  // Track locks
  omp_lock_t track_lock;
};

struct station_t {
  int station_num;
  string station_name;
  int last_arrival = UNDEFINED;
  int num_arrivals = 0;
  int total_waiting_time = UNDEFINED;
  int min_waiting_time = INF;
  int max_waiting_time = NINF;
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
  // If 0, means currently at a station.
  // If 0, means doors are closed, and can move if track is ready.
};

#endif //ASSIGN1_NETWORK_H
