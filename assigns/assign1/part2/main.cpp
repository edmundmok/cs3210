#include <unordered_map>
#include <mpi.h>

#include "read_utils.h"
#include "print_utils.h"
#include "network.h"

#define INPUT_FILE_NAME "in.txt"

int main(int argc, char* argv[]) {

  // Preliminary read to determine "correct" number of processes
  freopen(INPUT_FILE_NAME, "r", stdin);
  int S = read_integer_line(cin);
  vector<string> stations_strs;
  read_comma_sep_line(cin, stations_strs);

  int correct_num_procs = S;
  int mat_val;
  for (int i=0; i<S; i++){
    for (int j=0; j<S; j++) {
      cin >> mat_val;
      // don't double count the tracks
      if (i < j and mat_val) correct_num_procs++;
    }
  }

  // Add +1 proc for master coordinator
  correct_num_procs++;

  // Initialize OpenMPI up front otherwise there will be some issues
  MPI_Init(&argc, &argv);
  int my_id, num_procs, master;
  MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_id);

  // Determine if we should continue or just exit here
//  if (num_procs != correct_num_procs) {
//    printf("Incorrect number of processes used, should be %d for the given input. "
//             "Use num links (count shared as 1) + num stations + 1 (master). "
//             "Terminating.\n", correct_num_procs);
//    MPI_Finalize();
//    exit(0);
//  }

  master = num_procs - 1;
  if (my_id == master) {
    freopen(INPUT_FILE_NAME, "r", stdin);
    // Read number of train stations in network
    S = read_integer_line(cin);

    // Read list of stations and
    stations_strs.clear();
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

    int N = read_integer_line(cin);

    vector<string> num_trains;
    read_comma_sep_line(cin, num_trains);
    int num_greens = stoi(num_trains[0]),
      num_yellows = stoi(num_trains[1]),
      num_blues = stoi(num_trains[2]);

    TrainCounts train_counts(num_greens, num_yellows, num_blues);

    // Allocate stations to remaining processes

    // Allocate links to respective process

  }

  if (my_id < S) {
    // station processes

  } else if (my_id < master) {
    // link processes

  }

  MPI_Barrier(MPI_COMM_WORLD);
  
  // Run simulation
//  run_simulation(N, train_counts, blue_line, yellow_line, green_line,
//                 station_popularities, dist_matrix, station_use, track_use);

  MPI_Finalize();

  return 0;
}