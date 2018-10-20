#include <unordered_map>
#include <mpi.h>

#include "read_utils.h"
#include "print_utils.h"
#include "network.h"

#define INPUT_FILE_NAME "in.txt"
#define GREEN 'G'
#define YELLOW 'Y'
#define BLUE 'B'

void simulate(int N, int S, int my_id, int master) {
  for (int i=0; i<N; i++) {
    if (my_id < S) {
      // send state of trains
    } else if (my_id < master) {
      // send state of trains
    } else {
      // PRINT STATE OF ALL TRAINS

    }

    MPI_Barrier(MPI_COMM_WORLD);

    // ALL RUN ONE TICK

    MPI_Barrier(MPI_COMM_WORLD);

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
//  if (num_procs != correct_num_procs) {
//    if (my_id == master) {
//      printf("Incorrect number of processes used, should be %d for the given input. "
//               "Use num links (count shared as 1) + num stations + 1 (master). "
//               "Terminating.\n", correct_num_procs);
//    }
//    MPI_Finalize();
//    exit(0);
//  }

  int N;

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

    TrainCounts train_counts(num_greens, num_yellows, num_blues);

    AdjMatrix link_rank(S, vector<int> (S));

    // Allocate links to respective process
    int proc_rank = S;
    for (int i=0; i<S; i++) {
      for (int j=0; j<S; j++) {
        if (dist_matrix[i][j] == 0) continue;
//        // Send distance
//        MPI_Send(&dist_matrix[i][j], 1, MPI_INT, proc_rank, 0, MPI_COMM_WORLD);
//
//        // Send i and j
//        MPI_Send(&i, 1, MPI_INT, proc_rank, 0, MPI_COMM_WORLD);
//        MPI_Send(&j, 1, MPI_INT, proc_rank, 0, MPI_COMM_WORLD);

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


//    /* *
//     * DEBUG!
//     *
//     * */
//
//    cout << "lines:" << endl;
//    cout << "green: ";
//    print_vector(green_line);
//    cout << "yellow: ";
//    print_vector(yellow_line);
//    cout << "blue: ";
//    print_vector(blue_line);
//
//    cout << "link matrix:" << endl;
//    for (int i=0; i<S; i++) {
//      for (int j=0; j<S; j++) {
//        cout << link_rank[i][j] << " ";
//      }
//      cout << endl;
//    }
//
//    // print pairings for lines
//    // green line pairings
//    cout << "green: " << endl;
//    for (auto& pairing_list : green_line_pairings) {
//      cout << pairing_list.first << ":";
//      for (auto& pairs : pairing_list.second) {
//        cout << " (" << pairs.first << ", " << pairs.second << ")";
//      }
//      cout << endl;
//    }
//
//    // yellow line pairings
//    cout << "yellow: " << endl;
//    for (auto& pairing_list : yellow_line_pairings) {
//      cout << pairing_list.first << ":";
//      for (auto& pairs : pairing_list.second) {
//        cout << " (" << pairs.first << ", " << pairs.second << ")";
//      }
//      cout << endl;
//    }
//
//    // blue line pairings
//    cout << "blue: " << endl;
//    for (auto& pairing_list : blue_line_pairings) {
//      cout << pairing_list.first << ":";
//      for (auto& pairs : pairing_list.second) {
//        cout << " (" << pairs.first << ", " << pairs.second << ")";
//      }
//      cout << endl;
//    }


    // Allocate stations to remaining processes
    for (int i=0; i<S; i++) {
      // send train counts (g, y b)
      // send is head / tail for g, y, b
      // 0 for none, 1 for head, 2 for tail
      MPI_Send(&train_counts.num_greens, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
      MPI_Send(&train_counts.num_yellows, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
      MPI_Send(&train_counts.num_blues, 1, MPI_INT, i, 0, MPI_COMM_WORLD);

      int green_val, yellow_val, blue_val;
      green_val = yellow_val = blue_val = 0;

      if (i == green_line[0]) green_val = 1;
      else if (i == green_line[green_line.size()-1]) green_val = 2;

      if (i == yellow_line[0]) yellow_val = 1;
      else if (i == yellow_line[yellow_line.size()-1]) yellow_val = 2;

      if (i == blue_line[0]) blue_val = 1;
      else if (i == blue_line[blue_line.size()-1]) blue_val = 2;

      MPI_Send(&green_val, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
      MPI_Send(&yellow_val, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
      MPI_Send(&blue_val, 1, MPI_INT, i, 0, MPI_COMM_WORLD);

      // send station popularity
      MPI_Send(&station_popularities[i], 1, MPI_INT, i, 0, MPI_COMM_WORLD);

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
    MPI_Recv(&green_state, 1, MPI_INT, master, 0, MPI_COMM_WORLD, &status);
    MPI_Recv(&yellow_state, 1, MPI_INT, master, 0, MPI_COMM_WORLD, &status);
    MPI_Recv(&blue_state, 1, MPI_INT, master, 0, MPI_COMM_WORLD, &status);

    // station popularity
    MPI_Recv(&station.popularity, 1, MPI_INT, master, 0, MPI_COMM_WORLD, &status);

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
    }

    // initialize trains into queues asap!!
    if (green_state) {
      bool is_head = green_state == 1;

      // use num trains to determine how many to load for current term
      if (is_head) {
        for (int i=0; i<num_greens; i+=2) {
          Train train(GREEN, i);
          station.station_use_queue.push(train);
        }
      } else {
        for (int i=1; i<num_greens; i+=2) {
          Train train(GREEN, i);
          station.station_use_queue.push(train);
        }
      }
    }

    if (yellow_state) {
      bool is_head = yellow_state == 1;

      // use num trains to determine how many to load for current term
      if (is_head) {
        for (int i=0; i<num_yellows; i+=2) {
          Train train(YELLOW, i);
          station.station_use_queue.push(train);
        }
      } else {
        for (int i=1; i<num_yellows; i+=2) {
          Train train(YELLOW, i);
          station.station_use_queue.push(train);
        }
      }
    }

    if (blue_state) {
      bool is_head = blue_state == 1;

      // use num trains to determine how many to load for current term
      if (is_head) {
        for (int i=0; i<num_blues; i+=2) {
          Train train(BLUE, i);
          station.station_use_queue.push(train);
        }
      } else {
        for (int i=1; i<num_blues; i+=2) {
          Train train(BLUE, i);
          station.station_use_queue.push(train);
        }
      }
    }

    if (!station.station_use_queue.empty())
      station.generate_random_loading_time();

  } else if (my_id < master) {
//    // link processes
//    // get the link object from master
//    int dist, left, right;
//
//    // Get the distance
//    MPI_Recv(&track.dist, 1, MPI_INT, master, 0, MPI_COMM_WORLD, &status);
//    MPI_Recv(&track.source, 1, MPI_INT, master, 0, MPI_COMM_WORLD, &status);
//    MPI_Recv(&track.dest, 1, MPI_INT, master, 0, MPI_COMM_WORLD, &status);
//    track.remaining_time = track.dist;
  }


  MPI_Barrier(MPI_COMM_WORLD);

  // Send time ticks to all
  MPI_Bcast(&N, 1, MPI_INT, master, MPI_COMM_WORLD);

  MPI_Barrier(MPI_COMM_WORLD);

  simulate(N, S, my_id, master);

  // Run simulation
//  run_simulation(N, train_counts, blue_line, yellow_line, green_line,
//                 station_popularities, dist_matrix, station_use, track_use);

  MPI_Finalize();

  return 0;
}