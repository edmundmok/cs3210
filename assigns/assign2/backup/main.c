#include <stdio.h>
#include <stdint.h>

#include "nhash.h"
#include "utils.h"

int main() {

  freopen("./test.in", "r", stdin);

  char prev_digest_hex_str[65];
  uint64_t target;
  read_inputs(prev_digest_hex_str, &target);

  uint8_t input[52];
  uint32_t timestamp = 1541764931;
  generate_partial_hash_input(input, timestamp, prev_digest_hex_str);

  uint64_t nonce = 27601;
  fill_input_with_nonce(input, nonce);

  // Hash the input
  uint8_t hash[32];
  sha256(hash, input, 52);

  printf("VALID NONCE: %s\n", (check_if_valid_nonce(hash, target)) ? "YES" : "NO");

  // Print final output
  printf("Target: %llu\n", target);
  print_final_output(timestamp, nonce, hash);

  return 0;
}