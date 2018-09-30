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

struct TrainCounts;
struct Station;
struct train_t;

class Train;
class StationUse;
class TrackUse;

typedef vector<Station> Stations;
typedef vector<vector<int>> AdjMatrix;
typedef vector<Train> Trains;
typedef vector<float> Popularities;

typedef vector<StationUse> StationUses;
typedef vector<vector<TrackUse>> TrackUses;

enum TrainState { LOAD, MOVE };

enum TrainDirection { FORWARD, BACKWARD };

struct TrainCounts {
  int g;
  int y;
  int b;
  int total;
};

struct StationUse {
  // Last use times
  int forward_time = -1;
  int backward_time = -1;

  // Load queues
  queue<int> forward_load_q;
  queue<int> backward_load_q;
};

struct TrackUse {
  // Last use time
  int time = -1;

  // Track queues
  queue<int> track_q;
};

struct StationStats {
  int last_user = UNKNOWN_TRAIN;
  int last_arrival = UNDEFINED;
  int num_waits = 0;
  int total_wait_time = 0;
  int min_wait_time = INF;
  int max_wait_time = NINF;
};

struct Station {

  int station_num;
  string station_name;

  StationStats forward;
  StationStats backward;

  StationStats& get_stats(TrainDirection direction) {
    return (direction == FORWARD) ? forward : backward;
  }
};

int get_loading_time(int i, Popularities& popularity) {
  // use the formula and take ceiling because train must stop for at least
  // that amount but ticks are integers
  return ceil(popularity[i] * ((rand() % 10) + 1));
}

class Train {
public:
  char line;
  int lnum; // train number for this particular line
  int gnum;

  AdjMatrix& dist_matrix;
  Stations& stations;
  Popularities& station_popularities;

  TrainDirection direction;
  TrainState state;
  int local_station_num;
  int remaining_time;

  StationUses& station_uses;
  TrackUses& track_uses;

  int start_time; // For printing and debugging purposes

  Train(char line, int lnum, int gnum, Stations& stations,
        AdjMatrix& dist_matrix, Popularities& popularities,
        StationUses& station_use, TrackUses& track_uses) :
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

  Station& get_station() {
    return stations[local_station_num];
  }

  StationUse& get_station_use() {
    return station_uses[get_global_station_num()];
  }

  void queue_for_station_use() {
    StationUse& station_use = get_station_use();
    if (direction == FORWARD) station_use.forward_load_q.push(gnum);
    else station_use.backward_load_q.push(gnum);
  }

  TrackUse& get_track_use() {
    assert(state == MOVE);
    int curr_station = get_global_station_num(),
      next_station = get_global_next_station_num();
    return track_uses[curr_station][next_station];
  }

  void queue_for_track_use() {
    TrackUse& track_use = get_track_use();
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
    StationUse& station_use = get_station_use();
    return (direction == FORWARD and station_use.forward_load_q.front() == gnum
            and station_use.forward_time < tick)
           or (direction == BACKWARD and station_use.backward_load_q.front() == gnum
               and station_use.backward_time < tick);
  }

  void acknowledge_load(int tick) {
    StationUse& station_use = get_station_use();
    if (direction == FORWARD) station_use.forward_time = tick;
    else station_use.backward_time = tick;
  }

  bool has_door_just_opened() {
    return get_station().get_stats(direction).last_user != gnum;
  }

  bool is_first_arrival() {
    return get_station().get_stats(direction).last_arrival == UNDEFINED;
  }

  void update_station_wait_times_as_arrival(int tick) {
    StationStats& stats = get_station().get_stats(direction);
    stats.last_user = gnum;

    if (is_first_arrival()) return;

    int latest_wait = tick - stats.last_arrival;
    stats.total_wait_time += latest_wait;
    stats.num_waits++;
    stats.min_wait_time = min(stats.min_wait_time, latest_wait);
    stats.max_wait_time = max(stats.max_wait_time, latest_wait);
  }

  void update_remaining_time() { remaining_time--; }

  void update_station_wait_times_as_departure(int tick) {
    StationStats& stats = get_station().get_stats(direction);
    stats.last_arrival = tick;
  }

  void remove_as_station_user() {
    StationStats& stats = get_station().get_stats(direction);
    assert(stats.last_user == gnum);
    stats.last_user = UNDEFINED;
  }

  void dequeue_from_station_use() {
    assert(remaining_time == 0);
    assert(state == LOAD);

    StationUse& station_use = get_station_use();
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
    TrackUse& track_use = get_track_use();
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
    TrackUse& track_use = get_track_use();
    return (track_use.track_q.front() == gnum) and (track_use.time < tick);
  }

  void acknowledge_move_on_track(int tick) {
    TrackUse& track_use = get_track_use();
    track_use.time = tick;
  }

  void progress_to_load_at_next_station() {
    state = LOAD;
    local_station_num = get_local_next_station_num();
    reset_remaining_time_for_load();
  }

};


#endif //ASSIGN1_NETWORK_H
