#ifndef ASSIGN2_UTILS_H
#define ASSIGN2_UTILS_H

#include <stdint.h>
#include <stdio.h>
#include <endian.h>

void print_digest_prefix(uint8_t hash[]) {
  printf("The first 8 bytes of the digest are: \n");
  for (int i=0; i<8; i++) {
    printf("%02x", hash[i]);
  }
  printf("\n");
}

void print_hash_against_target_check(uint8_t hash[], uint64_t target) {
  // Only check 64 bits (8 bytes).
  uint64_t hash_prefix = be64toh(*(uint64_t *) hash);
//  printf("%llu\n", hash_prefix);
  bool is_valid = hash_prefix < target;
  printf("Valid nonce: %s\n", (is_valid) ? "Yes" : "No");
}

#endif //ASSIGN2_UTILS_H
