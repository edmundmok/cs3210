#ifndef ASSIGN2_UTILS_H
#define ASSIGN2_UTILS_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define NUSNET_ID "E0002744"
#define NONCE_IDX 44

void print_digest_prefix(uint8_t hash[]) {
  printf("The first 8 bytes of the digest are: ");
  for (int i=0; i<8; i++) {
    printf("%02x", hash[i]);
  }
  printf("\n");
}

bool check_if_valid_nonce(uint8_t *hash, uint64_t target) {
  // Only check 64 bits (8 bytes).
  // Hash output from SHA256 is in big endian.
  uint64_t hash_prefix = be64toh(*(uint64_t *) hash);
  printf("Hash Prefix: %llu, Target: %llu\n", hash_prefix, target);
  return hash_prefix < target;
}

void read_inputs(char prev_digest_hex_str[], uint64_t *target) {
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

  /**
   * uint8_t arr
   *
   *  ---------------------------------------------------
   * | Timestamp | Prev digest hex | NUSNET ID |  Nonce  |
   * |-----------|-----------------|-----------|---------|
   * |   0 - 3   |     4 - 35      |  36 - 43  | 44 - 52 |
   *  ---------------------------------------------------
   */

  // 1. Fill in timestamp (this part is little endian for some reason)
  for (int i=3; i>=0; i--) {
    input[i] = timestamp & 0xff;
    timestamp >>= 8;
  }

  // Digest is 256 bits.
  uint8_t prev_digest[32];
  char mini_prev_digest[3];
  for (int i=0; i<64; i+=2) {
    strncpy(mini_prev_digest, prev_digest_hex_str+i, 2);
    mini_prev_digest[2] = '\0';
    prev_digest[i>>1] = (uint8_t) strtol(mini_prev_digest, NULL, 16);
  }

  // 2. Fill in the previous digest
  for (int i=0; i<32; i++) input[i+4] = prev_digest[i];
  // 3. Fill in NUSNET ID
  for (int i=0; i<8; i++) input[i+36] = NUSNET_ID[i];

}

void fill_input_with_nonce(uint8_t input[], uint64_t nonce) {
  for (int i=51; i>=NONCE_IDX; i--) {
    input[i] = nonce & 0xff;
    nonce >>= 8;
  }
}

void print_final_output(uint32_t timestamp, uint64_t nonce, uint8_t hash[]) {
  printf("%s\n", NUSNET_ID);
  printf("%u\n", timestamp);
  printf("%llu\n", nonce);
  for (int i=0; i<32; i++) printf("%02x", hash[i]);
  printf("\n");
}

#endif //ASSIGN2_UTILS_H
