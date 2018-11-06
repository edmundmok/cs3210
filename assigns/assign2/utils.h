#ifndef ASSIGN2_UTILS_H
#define ASSIGN2_UTILS_H

#include <stdint.h>
#include <stdio.h>

void print_digest_prefix(uint8_t hash[]) {
  printf("The first 8 bytes of the digest are: \n");
  for (int i=0; i<8; i++) {
    printf("%02x", hash[i]);
  }
  printf("\n");
}

void print_hash_against_target_check(uint8_t hash[], uint8_t target[]) {
  bool is_valid = false;
  // Only check 64 bits (8 bytes).
  for (int i=0; i<8; i++) {
    if (target[i] > hash[i]) {
      is_valid = true;
      break;
    } else if (hash[i] > target[i]) {
      is_valid = false;
      break;
    }
  }

  printf("Valid nonce: %s\n", (is_valid) ? "Yes" : "No");
}

#endif //ASSIGN2_UTILS_H
