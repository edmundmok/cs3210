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

#define STATION_LOCK_PREFIX "station_"
#define TRACK_LOCK_PREFIX "track_"

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
  int lnum; // train number for this particular line
  int gnum;

  vector<vector<int>>& dist_matrix;
  vector<station_t>& stations;
  vector<float>& station_popularities;

  TrainDirection direction;
  TrainState state;
  int local_station_num;
  int remaining_time;

  vector<station_queue_t>& station_uses;
  vector<vector<track_queue_t>>& track_uses;

  int start_time; // For printing and debugging purposes

  Train(char line, int lnum, int gnum, vector<station_t>& stations,
        vector<vector<int>>& dist_matrix, vector<float>& popularities,
        vector<station_queue_t>& station_use,
        vector<vector<track_queue_t>>& track_uses) :
  line(line), lnum(lnum), gnum(gnum), stations(stations), dist_matrix(dist_matrix),
  state(LOAD), station_popularities(popularities), station_uses(station_use),
  track_uses(track_uses) {
    int num_stations = stations.size();
    direction = (lnum % 2 == 0) ? FORWARD : BACKWARD;
    local_station_num = (lnum % 2 == 0) ? 0 : num_stations-1;
    int global_station_num = get_global_station_num();
    start_time = lnum/2;
    reset_remaining_time_for_load();
  };

  int get_global_station_num() {
    return stations[local_station_num].station_num;
  }

  int get_local_next_station_num() {
    return local_station_num + ((direction == FORWARD) ? 1 : -1);
  }

  int get_global_next_station_num() {
    assert(!is_at_terminal_station());
    return stations[get_local_next_station_num()].station_num;
  }

  string get_station_lock_name() {
    return STATION_LOCK_PREFIX + to_string(direction)
           + to_string(get_global_station_num());
  }

  string get_track_lock_name() {
    int curr_station = get_global_station_num();
    int next_station = get_global_next_station_num();
    return TRACK_LOCK_PREFIX + to_string(curr_station)
           + "-" + to_string(next_station);
  }

  station_t& get_station() {
    return stations[local_station_num];
  }

  station_queue_t& get_station_use() {
    return station_uses[get_global_station_num()];
  }

  void queue_for_station_use() {
    station_queue_t& station_use = get_station_use();
    if (direction == FORWARD) station_use.forward_load_q.push(gnum);
    else station_use.backward_load_q.push(gnum);
  }

  track_queue_t& get_track_use() {
    assert(state == MOVE);
    int curr_station = get_global_station_num(),
      next_station = get_global_next_station_num();
    return track_uses[curr_station][next_station];
  }

  void queue_for_track_use() {
    track_queue_t& track_use = get_track_use();
    track_use.track_q.push(gnum);
  }

  void reset_remaining_time_for_track() {
    int curr_station = get_global_station_num(),
      next_station = get_global_next_station_num();
//    cout << line << " " << curr_station << ", " << next_station << endl;
//    for (int i=0; i<dist_matrix.size(); i++) {
//      for (int j=0; j<dist_matrix[0].size(); j++) {
//        cout << dist_matrix[i][j] << " ";
//      }
//      cout << endl;
//    }
    remaining_time = dist_matrix[curr_station][next_station];
  }

  bool should_load(int tick) {
    assert(state == LOAD);
    station_queue_t& station_use = get_station_use();
    return (direction == FORWARD and station_use.forward_load_q.front() == gnum
            and station_use.forward_time < tick)
           or (direction == BACKWARD and station_use.backward_load_q.front() == gnum
               and station_use.backward_time < tick);
  }

  void acknowledge_load(int tick) {
    station_queue_t& station_use = get_station_use();
    if (direction == FORWARD) station_use.forward_time = tick;
    else station_use.backward_time = tick;
  }

  bool has_door_just_opened() {
    station_t& station = get_station();
    return (direction == FORWARD and station.last_forward_user != gnum)
      or (direction == BACKWARD and station.last_backward_user != gnum);
  }

  bool is_first_arrival() {
    station_t& station = get_station();
    return (direction == FORWARD and station.last_forward_arrival == UNDEFINED)
      or (direction == BACKWARD and station.last_backward_arrival == UNDEFINED);
  }

  void update_station_wait_times_as_arrival(int tick) {
    station_t& station = get_station();
    if (direction == FORWARD) {
      station.last_forward_user = gnum;
    } else {
      station.last_backward_user = gnum;
    }

    if (is_first_arrival()) return;

    if (direction == FORWARD) {
      int latest_wait = tick - station.last_forward_arrival;
      station.total_forward_waiting_time += latest_wait;
      station.num_forward_waits++;
      station.min_forward_waiting_time = min(station.min_forward_waiting_time, latest_wait);
      station.max_forward_waiting_time = max(station.max_forward_waiting_time, latest_wait);
    } else {
      int latest_wait = tick - station.last_backward_arrival;
      station.total_backward_waiting_time += latest_wait;
      station.num_backward_waits++;
      station.min_backward_waiting_time = min(station.min_backward_waiting_time, latest_wait);
      station.max_backward_waiting_time = max(station.max_backward_waiting_time, latest_wait);
    }
  }

  void update_remaining_time() { remaining_time--; }

  void update_station_wait_times_as_departure(int tick) {
    station_t& station = get_station();
    if (direction == FORWARD) station.last_forward_arrival = tick;
    else station.last_backward_arrival = tick;
  }

  void remove_as_station_user() {
    station_t& station = get_station();
    if (direction == FORWARD) {
      assert(station.last_forward_user == gnum);
      station.last_forward_user = UNDEFINED;
    } else {
      assert(station.last_backward_user == gnum);
      station.last_forward_user = UNDEFINED;
    }
  }

  void dequeue_from_station_use() {
    assert(remaining_time == 0);
    assert(state == LOAD);

    station_queue_t& station_use = get_station_use();
    if (direction == FORWARD) {
      assert(station_use.forward_load_q.front() == gnum);
      station_use.forward_load_q.pop();
    } else {
      assert(station_use.backward_load_q.front() == gnum);
      station_use.backward_load_q.pop();
    }
  }

  void dequeue_from_track_use() {
    assert(state == MOVE);
    track_queue_t& track_use = get_track_use();
    assert(track_use.track_q.front() == gnum);
    track_use.track_q.pop();
  }

  bool is_at_terminal_station() {
    return (direction == BACKWARD and local_station_num == 0)
      or (direction == FORWARD and local_station_num == stations.size()-1);
  }

  void reverse_train_direction() {
    direction = (direction == FORWARD) ? BACKWARD : FORWARD;
  }

  void reset_remaining_time_for_load() {
    remaining_time = get_loading_time(get_global_station_num(), station_popularities);
  }

  bool should_move_on_track(int tick) {
    track_queue_t& track_use = get_track_use();
    return (track_use.track_q.front() == gnum) and (track_use.time < tick);
  }

  void acknowledge_move_on_track(int tick) {
    track_queue_t& track_use = get_track_use();
    track_use.time = tick;
  }

  void progress_to_load_at_next_station() {
    state = LOAD;
    local_station_num = get_local_next_station_num();
    reset_remaining_time_for_load();
  }

};


#endif //ASSIGN1_NETWORK_H
