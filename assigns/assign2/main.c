#include <stdio.h>
#include <stdint.h>

#include "hash.h"
#include "utils.h"

#define INPUT_SIZE 52

int main() {

  freopen("./test.in", "r", stdin);

  char prev_digest_hex_str[65];
  uint64_t target;
  read_inputs(prev_digest_hex_str, &target);

  uint8_t input[INPUT_SIZE];
  uint32_t timestamp = 0x5bb16380;
  generate_partial_hash_input(input, timestamp, prev_digest_hex_str);

  uint8_t *input_d;
  cudaMalloc((void **) &input_d, INPUT_SIZE);
  cudaMemcpy(input_d, input, INPUT_SIZE, cudaMemcpyHostToDevice);

//  uint64_t nonce = 0xe69d030000000000;
//  fill_input_with_nonce(input, nonce);
//
//  // Hash the input
//  uint8_t hash[32];
//  sha256(hash, input, 52);
//
//  printf("VALID NONCE: %s\n", (check_if_valid_nonce(hash, target)) ? "YES" : "NO");

  // Print final output
  printf("Target: %llu\n", target);
//  print_final_output(timestamp, nonce, hash);

  cudaFree(input_d);

  return 0;
}