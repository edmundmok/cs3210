#include <unordered_map>
#include <mpi.h>
#include <assert.h>

#include "read_utils2.h"
#include "print_utils2.h"
#include "network2.h"

#define INPUT_FILE_NAME "in.txt"
#define GREEN 'g'
#define YELLOW 'y'
#define BLUE 'b'
#define DUMMY_TRAIN 0
#define REAL_TRAIN 1

void serialize_train(Train& train, int serialized_arr[]) {
  serialized_arr[0] = train.line;
  serialized_arr[1] = train.train_num;
}

void simulate(int N, int S, int my_id, int master, int total_trains,
              Track& track, Station& station, TrainCounts& train_counts) {
  MPI_Status status;
  int serialized_train[4];

  for (int i=0; i<N; i++) {
    if (my_id < S) {
      // station: send state of trains
      for (auto& pair: station.station_use_queue) {
        Train& train = pair.first;
        serialize_train(train, serialized_train);
        serialized_train[2] = my_id;
        MPI_Send(&serialized_train, 4, MPI_INT, master, 3, MPI_COMM_WORLD);
      }

    } else if (my_id < master) {
      // track: send state of trains
      for (Train& train: track.track_use_queue) {
        serialize_train(train, serialized_train);
        serialized_train[2] = track.source;
        serialized_train[3] = track.dest;
        MPI_Send(&serialized_train, 4, MPI_INT, master,
                 (track.track_use_queue.front().train_num == train.train_num
                 && track.track_use_queue.front().line == train.line) ? 4 : 3, MPI_COMM_WORLD);
      }
    } else {
      cout << i << ": ";
      // PRINT STATE OF ALL TRAINS
      for (int j=0; j<total_trains; j++) {
        MPI_Recv(&serialized_train, 4, MPI_INT, MPI_ANY_SOURCE,
                 MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        // Train number and Station
        cout << char(serialized_train[0]) << serialized_train[1] << "-s" << serialized_train[2];
        // link number
        if (status.MPI_TAG == 4) {
          cout << "->s" << serialized_train[3];
        }
        cout << ", ";
      }
      cout << "\n";
    }

    MPI_Barrier(MPI_COMM_WORLD);

    // ALL RUN ONE TICK

    if (my_id < S) {
      // station
      bool has_valid_msg = false;
      int next_track = -99;
      Train front_train(-99, -99);

      if (!station.station_use_queue.empty()) {
        assert(station.remaining_time > 0);
        station.remaining_time--;
        if (station.remaining_time == 0) {
          has_valid_msg = true;
          front_train = station.station_use_queue.front().first;
          serialize_train(front_train, serialized_train);
          int prev_track = station.station_use_queue.front().second;

          if (serialized_train[0] == GREEN) {
            next_track = station.green_listen_send[prev_track];
          } else if (serialized_train[0] == BLUE) {
            next_track = station.blue_listen_send[prev_track];
          } else {
            next_track = station.yellow_listen_send[prev_track];
          }

        }
      }

      bool real_train_sent = false;

      unordered_set<int> updated;
      // Send updates to all next tracks
      for (int blue_rank : station.blue_send) {
        if (updated.find(blue_rank) != updated.end()) continue;
        updated.insert(blue_rank);

        if (has_valid_msg and blue_rank == next_track and !real_train_sent) {
          MPI_Send(&serialized_train, 2, MPI_INT, blue_rank, REAL_TRAIN, MPI_COMM_WORLD);
          station.station_use_queue.pop_front();
          real_train_sent = true;
          continue;
        }

        MPI_Send(&serialized_train, 2, MPI_INT, blue_rank, DUMMY_TRAIN, MPI_COMM_WORLD);
      }

      for (int yellow_rank : station.yellow_send) {
        if (updated.find(yellow_rank) != updated.end()) continue;
        updated.insert(yellow_rank);

        if (has_valid_msg and yellow_rank == next_track and !real_train_sent) {
          MPI_Send(&serialized_train, 2, MPI_INT, yellow_rank, REAL_TRAIN, MPI_COMM_WORLD);
          station.station_use_queue.pop_front();
          real_train_sent = true;
          continue;
        }

        MPI_Send(&serialized_train, 2, MPI_INT, yellow_rank, DUMMY_TRAIN, MPI_COMM_WORLD);
      }


      for (int green_rank : station.green_send) {
        if (updated.find(green_rank) != updated.end()) continue;
        updated.insert(green_rank);

        if (has_valid_msg and green_rank == next_track and !real_train_sent) {
          MPI_Send(&serialized_train, 2, MPI_INT, green_rank, REAL_TRAIN, MPI_COMM_WORLD);
          station.station_use_queue.pop_front();
          real_train_sent = true;
          continue;
        }

        MPI_Send(&serialized_train, 2, MPI_INT, green_rank, DUMMY_TRAIN, MPI_COMM_WORLD);
      }

      unordered_set<int> received;
      // Receive all updates from prev tracks
      for (int blue_rank : station.blue_listen) {
        if (received.find(blue_rank) != received.end()) continue;
        received.insert(blue_rank);

        MPI_Recv(&serialized_train, 2, MPI_INT, blue_rank, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        if (status.MPI_TAG == DUMMY_TRAIN) continue;
        Train train(serialized_train[0], serialized_train[1]);
        station.station_use_queue.push_back(make_pair(train, blue_rank));
      }

      for (int green_rank: station.green_listen) {
        if (received.find(green_rank) != received.end()) continue;
        received.insert(green_rank);

        MPI_Recv(&serialized_train, 2, MPI_INT, green_rank, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        if (status.MPI_TAG == DUMMY_TRAIN) continue;
        Train train(serialized_train[0], serialized_train[1]);
        station.station_use_queue.push_back(make_pair(train, green_rank));
      }

      for (int yellow_rank: station.yellow_listen) {
        if (received.find(yellow_rank) != received.end()) continue;
        received.insert(yellow_rank);

        MPI_Recv(&serialized_train, 2, MPI_INT, yellow_rank, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        if (status.MPI_TAG == DUMMY_TRAIN) continue;
        Train train(serialized_train[0], serialized_train[1]);
        station.station_use_queue.push_back(make_pair(train, yellow_rank));
      }

      if (real_train_sent) {
        if (front_train.line == BLUE) {
          station.blue.last_door_close = i;
        } else if (front_train.line == YELLOW) {
          station.yellow.last_door_close = i;
        } else {
          station.green.last_door_close = i;
        }
      }

      // update station timings if a new train opened its doors
      if (!station.station_use_queue.empty()) {
        Train new_front_train = station.station_use_queue.front().first;
        if (new_front_train.line != front_train.line and new_front_train.train_num != front_train.train_num) {
          // a new train arrived.
          if (new_front_train.line == BLUE) {
            if (station.blue.last_door_close != UNDEFINED) {
              station.blue.num_waits++;
              int latest_wait = i - station.blue.last_door_close;
              station.blue.total_wait_time += latest_wait;
              station.blue.min_wait_time = min(station.blue.min_wait_time, latest_wait);
              station.blue.max_wait_time = max(station.blue.max_wait_time, latest_wait);
            }
          } else if (new_front_train.line == YELLOW) {
            if (station.yellow.last_door_close != UNDEFINED) {
              station.yellow.num_waits++;
              int latest_wait = i - station.yellow.last_door_close;
              station.yellow.total_wait_time += latest_wait;
              station.yellow.min_wait_time = min(station.yellow.min_wait_time, latest_wait);
              station.yellow.max_wait_time = max(station.yellow.max_wait_time, latest_wait);
            }
          } else {
            assert(new_front_train.line == GREEN);
            if (station.green.last_door_close != UNDEFINED) {
              station.green.num_waits++;
              int latest_wait = i - station.green.last_door_close;
              station.green.total_wait_time += latest_wait;
              station.green.min_wait_time = min(station.green.min_wait_time, latest_wait);
              station.green.max_wait_time = max(station.green.max_wait_time, latest_wait);
            }
          }
        }
      }

      // update train timings
      if (real_train_sent) {
        station.remaining_time = generate_random_loading_time(station.popularity);
      }


    } else if (my_id < master) {
      // track
      bool received_train = false;
      MPI_Recv(&serialized_train, 2, MPI_INT, track.source, MPI_ANY_TAG,
               MPI_COMM_WORLD, &status);

      if (status.MPI_TAG == REAL_TRAIN) {
        received_train = true;
        Train train(serialized_train[0], serialized_train[1]);
        track.track_use_queue.push_back(train);
      }

      bool has_valid_msg = false;
      if (not track.track_use_queue.empty() and
        (not received_train
         or (received_train and serialized_train[0] != track.track_use_queue.front().line
             and serialized_train[1] != track.track_use_queue.front().train_num))) {
        assert(track.remaining_time > 0);
        track.remaining_time--;
        if (track.remaining_time == 0) {
          has_valid_msg = true;
          serialize_train(track.track_use_queue.front(), serialized_train);
          track.track_use_queue.pop_front();
        }
      }

      MPI_Send(&serialized_train, 2, MPI_INT, track.dest,
               (has_valid_msg) ? REAL_TRAIN : DUMMY_TRAIN, MPI_COMM_WORLD);

      if (track.remaining_time == 0 and not track.track_use_queue.empty()) {
        track.remaining_time = track.dist;
      }
    }

    MPI_Barrier(MPI_COMM_WORLD);

  }

  int serialized_station_stat[5];

  // Gather all timings
  if (my_id == master) {
    int blue_time = 0, green_time = 0, yellow_time = 0;
    int blue_waits = 0, green_waits = 0, yellow_waits = 0;
    int blue_min = INF, green_min = INF, yellow_min = INF;
    int blue_max = NINF, green_max = NINF, yellow_max = NINF;

    for (int i=0; i<S; i++) {
      // We need to ignore terminals because they

      MPI_Recv(&serialized_station_stat, 5, MPI_INT, i, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      if (serialized_station_stat[1] > 0) {
        blue_waits += serialized_station_stat[1];
        blue_time += serialized_station_stat[2];
        blue_min = min(blue_min, serialized_station_stat[3]);
        blue_max = max(blue_max, serialized_station_stat[4]);
      }

      MPI_Recv(&serialized_station_stat, 5, MPI_INT, i, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      if (serialized_station_stat[1] > 0) {
        green_waits += serialized_station_stat[1];
        green_time += serialized_station_stat[2];
        green_min = min(green_min, serialized_station_stat[3]);
        green_max = max(green_max, serialized_station_stat[4]);
      }

      MPI_Recv(&serialized_station_stat, 5, MPI_INT, i, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      if (serialized_station_stat[1] > 0) {
        yellow_waits += serialized_station_stat[1];
        yellow_time += serialized_station_stat[2];
        yellow_min = min(yellow_min, serialized_station_stat[3]);
        yellow_max = max(yellow_max, serialized_station_stat[4]);
      }
    }

    cout << "Averge waiting times:\n";
    cout << "green: " << train_counts.num_greens << " trains -> "
         << (green_time / float(green_waits)) << ", "
         << green_max << ", "
         << green_min << "\n";
    cout << "yellow: " << train_counts.num_yellows << " trains -> "
         << (yellow_time / float(yellow_waits)) << ", "
         << yellow_max << ", "
         << yellow_min << "\n";
    cout << "blue: " << train_counts.num_blues << " trains -> "
         << (blue_time / float(blue_waits)) << ", "
         << blue_max << ", "
         << blue_min << "\n";

  } else if (my_id < S) {

    // Send blue
    serialized_station_stat[0] = station.blue.last_door_close;
    serialized_station_stat[1] = station.blue.num_waits;
    serialized_station_stat[2] = station.blue.total_wait_time;
    serialized_station_stat[3] = station.blue.min_wait_time;
    serialized_station_stat[4] = station.blue.max_wait_time;

    MPI_Send(&serialized_station_stat, 5, MPI_INT, master, 0, MPI_COMM_WORLD);

    // Send green
    serialized_station_stat[0] = station.green.last_door_close;
    serialized_station_stat[1] = station.green.num_waits;
    serialized_station_stat[2] = station.green.total_wait_time;
    serialized_station_stat[3] = station.green.min_wait_time;
    serialized_station_stat[4] = station.green.max_wait_time;

    MPI_Send(&serialized_station_stat, 5, MPI_INT, master, 0, MPI_COMM_WORLD);

    // Send yellow
    serialized_station_stat[0] = station.yellow.last_door_close;
    serialized_station_stat[1] = station.yellow.num_waits;
    serialized_station_stat[2] = station.yellow.total_wait_time;
    serialized_station_stat[3] = station.yellow.min_wait_time;
    serialized_station_stat[4] = station.yellow.max_wait_time;

    MPI_Send(&serialized_station_stat, 5, MPI_INT, master, 0, MPI_COMM_WORLD);
  }
}

int main(int argc, char* argv[]) {

  // Initialize OpenMPI up front otherwise there will be some issues
  MPI_Init(&argc, &argv);
  int my_id, num_procs, master;
  MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_id);
  master = num_procs - 1;

  int correct_num_procs, S;

  if (my_id == master) {
    // Preliminary read to determine "correct" number of processes
    freopen(INPUT_FILE_NAME, "r", stdin);
    S = read_integer_line(cin);
    vector<string> stations_strs;
    read_comma_sep_line(cin, stations_strs);

    correct_num_procs = S;
    int mat_val;
    for (int i=0; i<S; i++){
      for (int j=0; j<S; j++) {
        cin >> mat_val;
        if (mat_val) correct_num_procs++;
      }
    }
    // Add +1 proc for master coordinator
    correct_num_procs++;

  }

  // Broadcast validation num procs to everyone
  MPI_Bcast(&correct_num_procs, 1, MPI_INT, master, MPI_COMM_WORLD);
  MPI_Bcast(&S, 1, MPI_INT, master, MPI_COMM_WORLD);

  // Determine if we should continue or just exit here
  if (num_procs != correct_num_procs) {
    if (my_id == master) {
      printf("Incorrect number of processes used, should be %d for the given input. "
               "Use unidirectional num links (count shared as 1) + num stations + 1 (master). "
               "Terminating.\n", correct_num_procs);
    }
    MPI_Finalize();
    exit(0);
  }

  int N, total_trains;
  TrainCounts train_counts;

  if (my_id == master) {
    freopen(INPUT_FILE_NAME, "r", stdin);
    // Read number of train stations in network
    S = read_integer_line(cin);

    // Read list of stations and
    vector<string> stations_strs;
    read_comma_sep_line(cin, stations_strs);
    // map station name to index number
    unordered_map<string, int> stations_map;
    get_station_map(stations_map, stations_strs);

    // Read adjacency matrix of network and setup dist_matrix
    AdjMatrix dist_matrix(S, vector<int>(S));

    for (int i=0; i<S; i++){
      for (int j=0; j<S; j++) {
        cin >> dist_matrix[i][j];
      }
    }
    cin.ignore(1, '\n');

    // Read station popularities
    Popularities station_popularities(S);
    read_popularities(cin, station_popularities);

    // Read stations for each line
    vector<int> green_line, yellow_line, blue_line;
    read_stations_for_line(cin, stations_strs, stations_map, dist_matrix, green_line);
    read_stations_for_line(cin, stations_strs, stations_map, dist_matrix, yellow_line);
    read_stations_for_line(cin, stations_strs, stations_map, dist_matrix, blue_line);

    N = read_integer_line(cin);

    vector<string> num_trains;
    read_comma_sep_line(cin, num_trains);
    int num_greens = stoi(num_trains[0]),
      num_yellows = stoi(num_trains[1]),
      num_blues = stoi(num_trains[2]);

//    TrainCounts train_counts(num_greens, num_yellows, num_blues);
    train_counts.num_greens = num_greens;
    train_counts.num_yellows = num_yellows;
    train_counts.num_blues = num_blues;
    train_counts.num_total = num_greens + num_yellows + num_blues;
    total_trains = train_counts.num_total;

    AdjMatrix link_rank(S, vector<int> (S));

    // Allocate links to respective process
    int proc_rank = S;
    for (int i=0; i<S; i++) {
      for (int j=0; j<S; j++) {
        if (dist_matrix[i][j] == 0) continue;
        // Send distance
        MPI_Send(&dist_matrix[i][j], 1, MPI_INT, proc_rank, 0, MPI_COMM_WORLD);

        // Send i and j
        MPI_Send(&i, 1, MPI_INT, proc_rank, 0, MPI_COMM_WORLD);
        MPI_Send(&j, 1, MPI_INT, proc_rank, 0, MPI_COMM_WORLD);

        link_rank[i][j] = proc_rank++;
      }
    }

    // Identify link pairings for each station first
    // pairs are (listen, send)
    unordered_map<int, vector<pair<int, int>>> green_line_pairings,
      yellow_line_pairings, blue_line_pairings;

    // Green line
    if (green_line.size() >= 2) {
      // Deal with terminal stations first
      green_line_pairings[green_line[0]].push_back(make_pair(
        link_rank[green_line[1]][green_line[0]],
        link_rank[green_line[0]][green_line[1]]
      ));
      green_line_pairings[green_line[green_line.size()-1]].push_back(make_pair(
        link_rank[green_line[green_line.size()-2]][green_line[green_line.size()-1]],
      link_rank[green_line[green_line.size()-1]][green_line[green_line.size()-2]]
      ));

      // Deal with non-terminal stations
      for (int i=1; i<green_line.size()-1; i++) {
        green_line_pairings[green_line[i]].push_back(make_pair(
          link_rank[green_line[i-1]][green_line[i]],
          link_rank[green_line[i]][green_line[i+1]]
        ));
        green_line_pairings[green_line[i]].push_back(make_pair(
          link_rank[green_line[i+1]][green_line[i]],
          link_rank[green_line[i]][green_line[i-1]]
        ));
      }
    }


    // Yellow line
    if (yellow_line.size() >= 2) {
      // Deal with terminal stations first
      yellow_line_pairings[yellow_line[0]].push_back(make_pair(
        link_rank[yellow_line[1]][yellow_line[0]],
      link_rank[yellow_line[0]][yellow_line[1]]
      ));
      yellow_line_pairings[yellow_line[yellow_line.size()-1]].push_back(make_pair(
        link_rank[yellow_line[yellow_line.size()-2]][yellow_line[yellow_line.size()-1]],
      link_rank[yellow_line[yellow_line.size()-1]][yellow_line[yellow_line.size()-2]]
      ));

      // Deal with non-terminal stations
      for (int i=1; i<yellow_line.size()-1; i++) {
        yellow_line_pairings[yellow_line[i]].push_back(make_pair(
          link_rank[yellow_line[i-1]][yellow_line[i]],
          link_rank[yellow_line[i]][yellow_line[i+1]]
        ));
        yellow_line_pairings[yellow_line[i]].push_back(make_pair(
          link_rank[yellow_line[i+1]][yellow_line[i]],
          link_rank[yellow_line[i]][yellow_line[i-1]]
        ));
      }
    }

    // Blue line
    if (blue_line.size() >= 2) {
      // Deal with terminal stations first
      blue_line_pairings[blue_line[0]].push_back(make_pair(
        link_rank[blue_line[1]][blue_line[0]],
        link_rank[blue_line[0]][blue_line[1]]
      ));
      blue_line_pairings[blue_line[blue_line.size()-1]].push_back(make_pair(
        link_rank[blue_line[blue_line.size()-2]][blue_line[blue_line.size()-1]],
        link_rank[blue_line[blue_line.size()-1]][blue_line[blue_line.size()-2]]
      ));

      // Deal with non-terminal stations
      for (int i=1; i<blue_line.size()-1; i++) {
        blue_line_pairings[blue_line[i]].push_back(make_pair(
          link_rank[blue_line[i-1]][blue_line[i]],
          link_rank[blue_line[i]][blue_line[i+1]]
        ));
        blue_line_pairings[blue_line[i]].push_back(make_pair(
          link_rank[blue_line[i+1]][blue_line[i]],
          link_rank[blue_line[i]][blue_line[i-1]]
        ));
      }
    }

    // Allocate stations to remaining processes
    for (int i=0; i<S; i++) {
      // send train counts (g, y b)
      // send is head / tail for g, y, b
      // 0 for none, 1 for head, 2 for tail
      MPI_Send(&train_counts.num_greens, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
      MPI_Send(&train_counts.num_yellows, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
      MPI_Send(&train_counts.num_blues, 1, MPI_INT, i, 0, MPI_COMM_WORLD);

      int green_val, yellow_val, blue_val;
      int green_tag, yellow_tag, blue_tag;
      green_val = yellow_val = blue_val = 0;
      green_tag = yellow_tag = blue_tag = 0;

      if (i == green_line[0]) {
        green_val = 1;
        green_tag = link_rank[green_line[1]][i];
      }
      else if (i == green_line[green_line.size()-1]) {
        green_val = 2;
        green_tag = link_rank[green_line[green_line.size()-2]][i];
      }

      if (i == yellow_line[0]) {
        yellow_val = 1;
        yellow_tag = link_rank[yellow_line[1]][i];
      }
      else if (i == yellow_line[yellow_line.size()-1]) {
        yellow_val = 2;
        yellow_tag = link_rank[yellow_line[yellow_line.size()-2]][i];
      }

      if (i == blue_line[0]) {
        blue_val = 1;
        blue_tag = link_rank[blue_line[1]][i];
      }
      else if (i == blue_line[blue_line.size()-1]) {
        blue_val = 2;
        blue_tag = link_rank[blue_line[blue_line.size()-2]][i];
      }

      // Send station state (0, 1 head, 2 tail)
      // station tag is depenedent on terminal state
      MPI_Send(&green_val, 1, MPI_INT, i, green_tag, MPI_COMM_WORLD);
      MPI_Send(&yellow_val, 1, MPI_INT, i, yellow_tag, MPI_COMM_WORLD);
      MPI_Send(&blue_val, 1, MPI_INT, i, blue_tag, MPI_COMM_WORLD);

      // send station popularity
      MPI_Send(&station_popularities[i], 1, MPI_FLOAT, i, 0, MPI_COMM_WORLD);

      // send pairings by batches
      // (Num) of greens
      int num = green_line_pairings[i].size();
      MPI_Send(&num, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
      for (auto& pair: green_line_pairings[i]) {
        // send "listen" half
        MPI_Send(&pair.first, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        // send "send" half
        MPI_Send(&pair.second, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
      }

      // yellow pairings
      num = yellow_line_pairings[i].size();
      MPI_Send(&num, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
      for (auto& pair: yellow_line_pairings[i]) {
        // send "listen" half
        MPI_Send(&pair.first, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        // send "send" half
        MPI_Send(&pair.second, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
      }

      // blue pairings
      num = blue_line_pairings[i].size();
      MPI_Send(&num, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
      for (auto& pair: blue_line_pairings[i]) {
        // send "listen" half
        MPI_Send(&pair.first, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        // send "send" half
        MPI_Send(&pair.second, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
      }
    }

  }

  MPI_Status status;
  Track track;
  Station station;
  if (my_id < S) {
    // station processes

    int num_greens, num_yellows, num_blues;
    MPI_Recv(&num_greens, 1, MPI_INT, master, 0, MPI_COMM_WORLD, &status);
    MPI_Recv(&num_yellows, 1, MPI_INT, master, 0, MPI_COMM_WORLD, &status);
    MPI_Recv(&num_blues, 1, MPI_INT, master, 0, MPI_COMM_WORLD, &status);

    // station state (0, 1 head, 2 tail)
    int green_state, yellow_state, blue_state;
    int green_tag, yellow_tag, blue_tag;
    MPI_Recv(&green_state, 1, MPI_INT, master, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    green_tag = status.MPI_TAG;
    MPI_Recv(&yellow_state, 1, MPI_INT, master, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    yellow_tag = status.MPI_TAG;
    MPI_Recv(&blue_state, 1, MPI_INT, master, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    blue_tag = status.MPI_TAG;

    // station popularity
    MPI_Recv(&station.popularity, 1, MPI_FLOAT, master, 0, MPI_COMM_WORLD, &status);

    int num, listen, send;
    // green pairings
    MPI_Recv(&num, 1, MPI_INT, master, 0, MPI_COMM_WORLD, &status);
    for (int i=0; i<num; i++) {
      // receive "listen" half
      MPI_Recv(&listen, 1, MPI_INT, master, 0, MPI_COMM_WORLD, &status);

      // receive "send" half
      MPI_Recv(&send, 1, MPI_INT, master, 0, MPI_COMM_WORLD, &status);

      station.green_listen.push_back(listen);
      station.green_send.push_back(send);
      station.green_listen_send[listen] = send;
    }

    // yellow pairings
    MPI_Recv(&num, 1, MPI_INT, master, 0, MPI_COMM_WORLD, &status);
    for (int i=0; i<num; i++) {
      // receive "listen" half
      MPI_Recv(&listen, 1, MPI_INT, master, 0, MPI_COMM_WORLD, &status);

      // receive "send" half
      MPI_Recv(&send, 1, MPI_INT, master, 0, MPI_COMM_WORLD, &status);

      station.yellow_listen.push_back(listen);
      station.yellow_send.push_back(send);
      station.yellow_listen_send[listen] = send;
    }


    // blue pairings
    MPI_Recv(&num, 1, MPI_INT, master, 0, MPI_COMM_WORLD, &status);
    for (int i=0; i<num; i++) {
      // receive "listen" half
      MPI_Recv(&listen, 1, MPI_INT, master, 0, MPI_COMM_WORLD, &status);

      // receive "send" half
      MPI_Recv(&send, 1, MPI_INT, master, 0, MPI_COMM_WORLD, &status);

      station.blue_listen.push_back(listen);
      station.blue_send.push_back(send);
      station.blue_listen_send[listen] = send;
    }

    // initialize trains into queues asap!!
    if (green_state) {
      bool is_head = green_state == 1;

      // use num trains to determine how many to load for current term
      if (is_head) {
        for (int i=0; i<num_greens; i+=2) {
          Train train(GREEN, i);
          station.station_use_queue.push_back(make_pair(train, green_tag));
        }
      } else {
        for (int i=1; i<num_greens; i+=2) {
          Train train(GREEN, i);
          station.station_use_queue.push_back(make_pair(train, green_tag));
        }
      }
    }

    if (yellow_state) {

      bool is_head = yellow_state == 1;

      // use num trains to determine how many to load for current term
      if (is_head) {
        for (int i=0; i<num_yellows; i+=2) {
          Train train(YELLOW, i);
          station.station_use_queue.push_back(make_pair(train, yellow_tag));
        }
      } else {
        for (int i=1; i<num_yellows; i+=2) {
          Train train(YELLOW, i);
          station.station_use_queue.push_back(make_pair(train, yellow_tag));
        }
      }
    }

    if (blue_state) {

      bool is_head = blue_state == 1;

      // use num trains to determine how many to load for current term
      if (is_head) {
        for (int i=0; i<num_blues; i+=2) {
          Train train(BLUE, i);
          station.station_use_queue.push_back(make_pair(train, blue_tag));
        }
      } else {
        for (int i=1; i<num_blues; i+=2) {
          Train train(BLUE, i);
          station.station_use_queue.push_back(make_pair(train, blue_tag));
        }
      }
    }
    station.remaining_time = generate_random_loading_time(station.popularity);

  } else if (my_id < master) {
    // link processes
    // get the link object from master
    int dist, left, right;

    // Get the distance
    MPI_Recv(&track.dist, 1, MPI_INT, master, 0, MPI_COMM_WORLD, &status);
    MPI_Recv(&track.source, 1, MPI_INT, master, 0, MPI_COMM_WORLD, &status);
    MPI_Recv(&track.dest, 1, MPI_INT, master, 0, MPI_COMM_WORLD, &status);
    track.remaining_time = track.dist;
  }

  MPI_Barrier(MPI_COMM_WORLD);

  // Send time ticks to all
  MPI_Bcast(&N, 1, MPI_INT, master, MPI_COMM_WORLD);

  MPI_Barrier(MPI_COMM_WORLD);

  simulate(N, S, my_id, master, total_trains, track, station, train_counts);

  MPI_Finalize();

  return 0;
}
