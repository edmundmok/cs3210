#ifndef ASSIGN2_UTILS_H
#define ASSIGN2_UTILS_H

#include <stdint.h>
#include <stdio.h>
#include <endian.h>

#define NUSNET_ID "E1234567"
#define NONCE_IDX 44

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

void read_inputs(char prev_digest_hex_str[], int *target) {
  printf("Enter previous digest (256-bit hex):\n");
  scanf("%s", prev_digest_hex_str);
  printf("Enter target value (64-bit decimal):\n");
  scanf("%llu", target);
}

void print_complete_hash_input(uint8_t input[]) {
  for (int i=0; i<52; i++) {
    printf("%02x", input[i]);
  }
  printf("\n");
}

void generate_partial_hash_input(uint8_t input[], uint32_t timestamp,
                                 char prev_digest_hex_str[]) {
  // Fill in the input
  // 1. Fill in timestamp (starting from LSB back up)
  for (int i=3; i>=0; i--) {
    input[i] = timestamp & 0xff;
    timestamp >>= 8;
  }

  // Digest is 256 bits.
  uint8_t prev_digest[32];

  // Convert digest str to uint8_t arr
  char mini_prev_digest[3];
  for (int i=0; i<64; i+=2) {
    strncpy(mini_prev_digest, prev_digest_hex_str+i, 2);
    mini_prev_digest[2] = '\0';
    prev_digest[i>>1] = (uint8_t) strtol(mini_prev_digest, NULL, 16);
  }

  // 2. Fill in the previous digest
  for (int i=0; i<32; i++) {
    input[i+4] = prev_digest[i];
  }

  // 3. Fill in NUSNET ID
  for (int i=0; i<8; i++) {
    input[i+36] = (uint8_t) NUSNET_ID[i];
  }

}

#endif //ASSIGN2_UTILS_H
