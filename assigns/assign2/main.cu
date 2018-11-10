#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "utils.h"
#include "hash.h"

#define INPUT_SIZE 52

__global__ void find_nonce_kernel(uint8_t *, uint64_t *target,
                                  int *, uint8_t *, uint64_t *);

int main() {

  freopen("./test.in", "r", stdin);

  char prev_digest_hex_str[65];
  uint64_t target;
  read_inputs(prev_digest_hex_str, &target);

  uint8_t input[INPUT_SIZE];
  time_t timestamp = time(NULL);
  generate_partial_hash_input(input, timestamp, prev_digest_hex_str);

  uint8_t *d_input;
  cudaMalloc((void **) &d_input, sizeof(input));
  cudaMemcpy(d_input, input, sizeof(input), cudaMemcpyHostToDevice);

  uint64_t *d_target;
  cudaMalloc((void **) &d_target, sizeof(uint64_t));
  cudaMemcpy(d_target, &target, sizeof(target), cudaMemcpyHostToDevice);

  int *d_found;
  int found = 0;
  cudaMalloc((void **) &d_found, sizeof(int));
  cudaMemcpy(d_found, &found, sizeof(int), cudaMemcpyHostToDevice);

  uint8_t hash[32];
  uint8_t *d_hash;
  cudaMalloc((void **) &d_hash, sizeof(hash));

  uint64_t *d_nonce;
  uint64_t nonce[1] = {0};
  cudaMalloc((void **) &d_nonce, sizeof(nonce));

  find_nonce_kernel<<<2, 64>>>(d_input, d_target, d_found, d_hash, d_nonce);

  // Copy input back
  cudaMemcpy(input, d_input, sizeof(input), cudaMemcpyDeviceToHost);

  // Copy nonce back
  cudaMemcpy(nonce, d_nonce, sizeof(nonce), cudaMemcpyDeviceToHost);

  // Copy digest/hash back
  cudaMemcpy(hash, d_hash, sizeof(hash), cudaMemcpyDeviceToHost);

  // Print final output
  printf("Target: %llu\n", target);
  print_final_output(timestamp, nonce[0], hash);

  cudaFree(d_input); cudaFree(d_target); cudaFree(d_found);
  cudaFree(d_nonce); cudaFree(d_hash);

  return 0;
}

__device__ void print_nonce(uint64_t nonce) {
  printf("%d\n", nonce);
}

__global__ void find_nonce_kernel(uint8_t *g_input, uint64_t *g_target,
                                  int *found, uint8_t *hash, uint64_t *g_nonce) {
  // Copy from global to local
  uint8_t l_input[52];
  uint8_t l_hash[32];

  uint64_t l_target = *g_target;

  for (int i=0; i<NONCE_IDX; i++) l_input[i] = g_input[i];

  size_t total_num_threads = gridDim.x * gridDim.y * gridDim.z
                             * blockDim.x * blockDim.y * blockDim.z;

  size_t block_index_in_grid = blockIdx.x * (gridDim.y * gridDim.z)
                               + blockIdx.y * (gridDim.z) + blockIdx.z;

  size_t thread_index_in_block = threadIdx.x * (blockDim.y * blockDim.z)
                                 + threadIdx.y * (blockDim.z) + threadIdx.z;

  size_t thread_id = block_index_in_grid * (blockDim.x * blockDim.y * blockDim.z)
                     + thread_index_in_block;

  uint64_t nonce = thread_id;

  // Start finding nonce
  while (!*found) {
//    print_nonce(nonce);
    fill_input_with_nonce(l_input, nonce);
    sha256(l_hash, l_input, 52);

    if (check_if_valid_nonce(l_hash, l_target) && *found == 0) {
      int old = atomicAdd(found, 1);
      if (old == 0) {
        // Only one thread can ever do this
        // Copy back input to global memory
        for (int i=0; i<INPUT_SIZE; i++) g_input[i] = l_input[i];

        // Copy hash to global memory
        for (int i=0; i<32; i++) hash[i] = l_hash[i];

        // Copy over nonce
        *g_nonce = nonce;
      }
    }
    nonce += total_num_threads;
  }

}