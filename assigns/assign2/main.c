#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "nhash.h"

#define NUM_BITS 256
//#define NUSNET_ID "E0002744"
#define NUSNET_ID "E1234567"
#define TEST_NONCE "16617441498601881600"

int main() {

  // Both digest and target (compared against hash) are 256 bits.
  uint8_t prev_digest[32], target[32];
  memset(prev_digest, sizeof(prev_digest), 0);
  memset(target, sizeof(target), 0);

  // 1 hex digit is 4 bits, 2 hex digit is 8 bits (uint8_t)
  char prev_digest_hex_str[65], target_dec_str[65];
  scanf("%s", prev_digest_hex_str);
  scanf("%s", target_dec_str);

  // Convert digest str to uint8_t arr
  char mini_prev_digest[3];
  for (int i=0; i<64; i+=2) {
    strncpy(mini_prev_digest, prev_digest_hex_str+i, 2);
    printf("%s\n", mini_prev_digest);
    prev_digest[i/2] = (uint8_t) strtol(mini_prev_digest, NULL, 16);
  }

  // The actual hash input can be of variable length.
  uint8_t input[53];
  memset(input, sizeof(input), 0);

  uint32_t timestamp = htonl(0x5bb16380);
  uint8_t hash[32];
//  uint8_t nonce = ;

  // Fill in the input
  // 1. Fill in timestamp (starting from LSB)
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
//    input[i+36] = (uint8_t) NUSNET_ID[i];
  }

  // 4. Fill in nonce


  // Hash the input
//  hash

  // Verify the result


  return 0;
}