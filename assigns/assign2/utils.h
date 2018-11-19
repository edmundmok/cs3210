#ifndef ASSIGN2_UTILS_H
#define ASSIGN2_UTILS_H

#define NUSNET_ID "E0002744"
#define NONCE_IDX 44

void print_digest_prefix(uint8_t hash[]);

__device__ inline uint64_t endian_swap(uint64_t x);

__device__ bool check_if_valid_nonce(uint8_t *hash, uint64_t target);

void read_inputs(char prev_digest_hex_str[], uint64_t *target);

void print_complete_hash_input(uint8_t input[]);

void generate_partial_hash_input(uint8_t input[], uint32_t timestamp,
                                 char prev_digest_hex_str[]);

__device__ void fill_input_with_nonce(uint8_t input[], uint64_t nonce);

void print_final_output(uint32_t timestamp, uint64_t nonce, uint8_t hash[]);

#endif //ASSIGN2_UTILS_H
