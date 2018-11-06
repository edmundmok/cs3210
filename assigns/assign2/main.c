#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "nhash.h"
#include "utils.h"

//#define NUSNET_ID "E0002744"
#define NUSNET_ID "E1234567"
//#define TEST_NONCE "16617441498601881600"
#define TEST_NONCE_HEX "e69d030000000000"

int main() {

  // 1 hex digit is 4 bits, 2 hex digit is 8 bits (uint8_t)
  char prev_digest_hex_str[65];
  uint64_t target;
  printf("Enter previous digest (256-bit hex):\n");
  scanf("%s", prev_digest_hex_str);
  printf("Enter target value (64-bit decimal):\n");
  scanf("%llu", &target);

  // Digest is 256 bits.
  uint8_t prev_digest[32];
  memset(prev_digest, sizeof(prev_digest), 0);

  // Convert digest str to uint8_t arr
  char mini_prev_digest[3];
  for (int i=0; i<64; i+=2) {
    strncpy(mini_prev_digest, prev_digest_hex_str+i, 2);
    prev_digest[i>>1] = (uint8_t) strtol(mini_prev_digest, NULL, 16);
  }

  // The actual hash input can be of variable length.
  uint8_t input[52];
  memset(input, sizeof(input), 0);

  uint32_t timestamp = 0x5bb16380;

  // Fill in the input
  // 1. Fill in timestamp (starting from LSB back up)
  for (int i=3; i>=0; i--) {
    input[i] = timestamp & 0xff;
    timestamp >>= 8;
  }

  // 2. Fill in the previous digest
  for (int i=0; i<32; i++) {
    input[i+4] = prev_digest[i];
  }

  // 3. Fill in NUSNET ID
  for (int i=0; i<8; i++) {
    input[i+36] = (uint8_t) NUSNET_ID[i];
  }

  // 4. Fill in nonce
  char mini_nounce[3];
  for (int i=0; i<16; i+=2) {
    strncpy(mini_nounce, TEST_NONCE_HEX+i, 2);
    input[(i>>1)+44] = (uint8_t) strtol(mini_nounce, NULL, 16);
  }

  // Hash the input
  uint8_t hash[32];
  sha256(hash, input, 52);

  // Verify the result
  print_digest_prefix(hash);

  // Compare against target
  print_hash_against_target_check(hash, target);

  return 0;
}