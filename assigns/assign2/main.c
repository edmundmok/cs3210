#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "nhash.h"
#include "utils.h"

int main() {

  freopen("./test.in", "r", stdin);

  // 1 hex digit is 4 bits
  char prev_digest_hex_str[65];
  uint64_t target;
  read_inputs(prev_digest_hex_str, &target);

  // The actual hash input can be of variable length.
  uint8_t input[52];
  generate_partial_hash_input(input, prev_digest_hex_str);

  // 4. Fill in nonce
  uint64_t nonce = 0xe69d030000000000;
  uint64_t *nonce_ptr = input+44;
  *nonce_ptr = htobe64(nonce);

  print_complete_hash_input(input);

  // Hash the input
  uint8_t hash[32];
  sha256(hash, input, 52);

  // Verify the result
  print_digest_prefix(hash);

  // Compare against target
  print_hash_against_target_check(hash, target);

  return 0;
}