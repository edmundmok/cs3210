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

class Train;
struct StationUse;
struct TrackUse;

typedef vector<Station> Stations;
typedef vector<vector<int>> AdjMatrix;
typedef vector<Train> Trains;
typedef vector<float> Popularities;

typedef vector<StationUse> StationUses;
typedef vector<vector<TrackUse>> TrackUses;

enum TrainState { LOAD, MOVE };

enum TrainDirection { FORWARD, BACKWARD };

struct TrainCounts {
  int num_greens;
  int num_yellows;
  int num_blues;
  int num_total;

  TrainCounts(int num_greens, int num_yellows, int num_blues) :
    num_greens(num_greens), num_yellows(num_yellows), num_blues(num_blues) {
    num_total = num_greens + num_yellows + num_blues;
  }
};

struct StationUsage {
  int last_use_time = -1;
  queue<int> load_queue;
};

struct StationUse {

  StationUsage forward;
  StationUsage backward;

  StationUsage& get_usage(TrainDirection& direction) {
    return (direction == FORWARD) ? forward : backward;
  }
};

struct TrackUse {
  // Last use time
  int last_use_time = -1;
  // Track queues
  queue<int> track_q;
};

struct StationStats {
  int last_user = UNKNOWN_TRAIN;
  int last_door_close = UNDEFINED;
  int num_waits = 0;
  int total_wait_time = 0;
  int min_wait_time = INF;
  int max_wait_time = NINF;
};

struct Station {

  int station_num;
  string& station_name;

  StationStats forward;
  StationStats backward;

  Station(int station_num, string& station_name) :
    station_num(station_num), station_name(station_name) {}

  StationStats& get_stats(TrainDirection direction) {
    return (direction == FORWARD) ? forward : backward;
  }
};

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
        StationUses& station_uses, TrackUses& track_uses) :
  line(line), lnum(lnum), gnum(gnum), stations(stations), dist_matrix(dist_matrix),
  state(LOAD), station_popularities(popularities), station_uses(station_uses),
  track_uses(track_uses) {
    direction = (lnum % 2 == 0) ? FORWARD : BACKWARD;
    local_station_num = (lnum % 2 == 0) ? 0 : stations.size() - 1;
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
    assert(state == LOAD);
    get_station_use().get_usage(direction).load_queue.push(gnum);
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
    remaining_time = dist_matrix[curr_station][next_station];
  }

  bool should_load(int tick) {
    assert(state == LOAD);
    StationUsage& usage = get_station_use().get_usage(direction);
    return (usage.load_queue.front() == gnum) and (usage.last_use_time < tick);
  }

  void acknowledge_load(int tick) {
    StationUsage& usage = get_station_use().get_usage(direction);
    assert(usage.last_use_time < tick);
    usage.last_use_time = tick;
  }

  bool has_door_just_opened() {
    return get_station().get_stats(direction).last_user != gnum;
  }

  bool is_first_door_open() {
    return get_station().get_stats(direction).last_door_close == UNDEFINED;
  }

  void update_station_wait_times_for_door_open(int tick) {
    StationStats& stats = get_station().get_stats(direction);
    stats.last_user = gnum;

    if (is_first_door_open()) return;

    int latest_wait = tick - stats.last_door_close;
    stats.total_wait_time += latest_wait;
    stats.num_waits++;
    stats.min_wait_time = min(stats.min_wait_time, latest_wait);
    stats.max_wait_time = max(stats.max_wait_time, latest_wait);
  }

  void update_remaining_time() { remaining_time--; }

  void update_station_wait_times_for_door_close(int tick) {
    StationStats& stats = get_station().get_stats(direction);
    stats.last_door_close = tick;
  }

  void remove_as_station_user() {
    StationStats& stats = get_station().get_stats(direction);
    assert(stats.last_user == gnum);
    stats.last_user = UNDEFINED;
  }

  void dequeue_from_station_use() {
    assert(remaining_time == 0);
    assert(state == LOAD);

    StationUsage& usage = get_station_use().get_usage(direction);
    assert(usage.load_queue.front() == gnum);
    usage.load_queue.pop();
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

  int generate_random_loading_time() {
    // use the formula and take ceiling because train must stop for at least
    // that amount but ticks are integers
    return ceil(station_popularities[get_global_station_num()]
                * ((rand() % 10) + 1));
  }

  void reset_remaining_time_for_load() {
    remaining_time = generate_random_loading_time();
  }

  bool should_move_on_track(int tick) {
    TrackUse& track_use = get_track_use();
    return (track_use.track_q.front() == gnum)
           and (track_use.last_use_time < tick);
  }

  void acknowledge_move_on_track(int tick) {
    TrackUse& track_use = get_track_use();
    track_use.last_use_time = tick;
  }

  void progress_to_load_at_next_station() {
    state = LOAD;
    local_station_num = get_local_next_station_num();
    reset_remaining_time_for_load();
  }

  bool is_actually_moving_on_track() {
    assert(state == MOVE);
    return get_track_use().track_q.front() == gnum;
  }

};


#endif //ASSIGN1_NETWORK_H
