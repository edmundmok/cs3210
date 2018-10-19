#include <unordered_map>

#include "mpi.h"
#include "read_utils.h"
#include "print_utils.h"
#include "network.h"

int main(int argc, char* argv[]) {
  // Read number of train stations in network
  int S = read_integer_line(cin);

  // Read list of stations and
  vector<string> stations_strs;
  read_comma_sep_line(cin, stations_strs);
  // map station name to index number
  unordered_map<string, int> stations_map;
  get_station_map(cin, stations_map, stations_strs);

  // Read adjacency matrix of network and
  // setup dist_matrix
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

  // Should'nt we enforce the number of processes?
  MPI_Init(&argc, &argv);

  int rank, num_tasks;
  MPI_Comm_size(MPI_COMM_WORLD, &num_tasks);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // Allocate links to each process

  // Allocate stations to remaining processes

  // Run simulation
//  run_simulation(N, train_counts, blue_line, yellow_line, green_line,
//                 station_popularities, dist_matrix, station_use, track_use);

  MPI_Finalize();

  return 0;
}