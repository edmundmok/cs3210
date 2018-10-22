#ifndef ASSIGN1_NETWORK_H
#define ASSIGN1_NETWORK_H

#include <vector>
#include <deque>
#include <cmath>

#define UNDEFINED (-99)
#define INF (999999999)
#define NINF (-999999999)

using namespace std;

struct TrainCounts;
struct Station;

typedef vector<float> Popularities;
typedef vector<Station> Stations;
typedef vector<vector<int>> AdjMatrix;

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

struct Train {

  char line;
  int train_num;

  Train(char line, int train_num) : line(line), train_num(train_num) {}
};

struct StationStats {

//  bool is_valid = false;
  int last_door_close = UNDEFINED;
  int num_waits = 0;
  int total_wait_time = 0;
  int min_wait_time = INF;
  int max_wait_time = NINF;

};

struct Station {

  /**
   * Stations are
   */

  // direction does not matter, only one train can load at a time
  deque<pair<Train, int>> station_use_queue;

  // remaining time for train at the front
  int remaining_time;

  float popularity;

  // pairings for "listen" and "send" per line
  vector<int> blue_listen, green_listen, yellow_listen;
  vector<int> blue_send, green_send, yellow_send;
  unordered_map<int, int> blue_listen_send, green_listen_send, yellow_listen_send;

  // handle timings for each line (if valid)
  StationStats blue;
  StationStats green;
  StationStats yellow;

};

int generate_random_loading_time(float popularity) {
  // use the formula and take ceiling because train must stop for at least
  // that amount but ticks are integers
  return ceil(popularity * ((rand() % 10) + 1));
}

struct Track {
  /**
   * Tracks are unidirectional so just need one queue per track.
   */

  int dist;

  deque<Train> track_use_queue;
  int remaining_time = 0;

  // neighbor proc ranks
  int source;
  int dest;

};

#endif //ASSIGN1_NETWORK_H
