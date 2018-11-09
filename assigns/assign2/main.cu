#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "utils.h"

#define INPUT_SIZE 52


#define CHUNK_SIZE 64
#define TOTAL_LEN_LEN 8

__global__ void find_nonce_kernel(uint8_t *, uint64_t *target,
                                  int *, uint8_t *, uint64_t *);

/*
 * Initialize array of round constants:
 * (first 32 bits of the fractional parts of the cube roots of the first 64 primes 2..311):
 */
__constant__ uint32_t k[] = {
0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

struct buffer_state {
  const uint8_t * p;
  size_t len;
  size_t total_len;
  bool single_one_delivered;
  bool total_len_delivered;
};

__device__ static uint32_t right_rot(uint32_t value, unsigned int count)
{
/*
 * Defined behaviour in standard C for all count where 0 < count < 32,
 * which is what we need here.
 */
return value >> count | value << (32 - count);
}

__device__ static void init_buf_state(struct buffer_state * state, const uint8_t * input, size_t len)
{
  state->p = input;
  state->len = len;
  state->total_len = len;
  state->single_one_delivered = false;
  state->total_len_delivered = false;
}

__device__ static bool calc_chunk(uint8_t chunk[CHUNK_SIZE], struct buffer_state * state)
{
  size_t space_in_chunk;

  if (state->total_len_delivered) {
    return false;
  }

  if (state->len >= CHUNK_SIZE) {
    memcpy(chunk, state->p, CHUNK_SIZE);
    state->p += CHUNK_SIZE;
    state->len -= CHUNK_SIZE;
    return true;
  }

  memcpy(chunk, state->p, state->len);
  chunk += state->len;
  space_in_chunk = CHUNK_SIZE - state->len;
  state->p += state->len;
  state->len = 0;

  /* If we are here, space_in_chunk is one at minimum. */
  if (!state->single_one_delivered) {
    *chunk++ = 0x80;
    space_in_chunk -= 1;
    state->single_one_delivered = true;
  }

  /*
   * Now:
   * - either there is enough space left for the total length, and we can conclude,
   * - or there is too little space left, and we have to pad the rest of this chunk with zeroes.
   * In the latter case, we will conclude at the next invokation of this function.
   */
  if (space_in_chunk >= TOTAL_LEN_LEN) {
    const size_t left = space_in_chunk - TOTAL_LEN_LEN;
    size_t len = state->total_len;
    int i;
    memset(chunk, 0x00, left);
    chunk += left;

    /* Storing of len * 8 as a big endian 64-bit without overflow. */
    chunk[7] = (uint8_t) (len << 3);
    len >>= 5;
    for (i = 6; i >= 0; i--) {
      chunk[i] = (uint8_t) len;
      len >>= 8;
    }
    state->total_len_delivered = true;
  } else {
    memset(chunk, 0x00, space_in_chunk);
  }

  return true;
}

__device__ void sha256(uint8_t hash[32], const uint8_t * input, size_t len)
{
  /*
   * Note 1: All integers (expect indexes) are 32-bit unsigned integers and addition is calculated modulo 2^32.
   * Note 2: For each round, there is one round constant k[i] and one entry in the message schedule array w[i], 0 = i = 63
   * Note 3: The compression function uses 8 working variables, a through h
   * Note 4: Big-endian convention is used when expressing the constants in this pseudocode,
   *     and when parsing message block data from bytes to words, for example,
   *     the first word of the input message "abc" after padding is 0x61626380
   */

  /*
   * Initialize hash values:
   * (first 32 bits of the fractional parts of the square roots of the first 8 primes 2..19):
   */
  uint32_t h[] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
  int i, j;

  /* 512-bit chunks is what we will operate on. */
  uint8_t chunk[64];

  struct buffer_state state;

  init_buf_state(&state, input, len);

  while (calc_chunk(chunk, &state)) {
    uint32_t ah[8];

    /*
     * create a 64-entry message schedule array w[0..63] of 32-bit words
     * (The initial values in w[0..63] don't matter, so many implementations zero them here)
     * copy chunk into first 16 words w[0..15] of the message schedule array
     */
    uint32_t w[64];
    const uint8_t *p = chunk;

    memset(w, 0x00, sizeof w);
    for (i = 0; i < 16; i++) {
      w[i] = (uint32_t) p[0] << 24 | (uint32_t) p[1] << 16 |
             (uint32_t) p[2] << 8 | (uint32_t) p[3];
      p += 4;
    }

    /* Extend the first 16 words into the remaining 48 words w[16..63] of the message schedule array: */
    for (i = 16; i < 64; i++) {
      const uint32_t s0 = right_rot(w[i - 15], 7) ^ right_rot(w[i - 15], 18) ^ (w[i - 15] >> 3);
      const uint32_t s1 = right_rot(w[i - 2], 17) ^ right_rot(w[i - 2], 19) ^ (w[i - 2] >> 10);
      w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    /* Initialize working variables to current hash value: */
    for (i = 0; i < 8; i++)
      ah[i] = h[i];

    /* Compression function main loop: */
    for (i = 0; i < 64; i++) {
      const uint32_t s1 = right_rot(ah[4], 6) ^ right_rot(ah[4], 11) ^ right_rot(ah[4], 25);
      const uint32_t ch = (ah[4] & ah[5]) ^ (~ah[4] & ah[6]);
      const uint32_t temp1 = ah[7] + s1 + ch + k[i] + w[i];
      const uint32_t s0 = right_rot(ah[0], 2) ^ right_rot(ah[0], 13) ^ right_rot(ah[0], 22);
      const uint32_t maj = (ah[0] & ah[1]) ^ (ah[0] & ah[2]) ^ (ah[1] & ah[2]);
      const uint32_t temp2 = s0 + maj;

      ah[7] = ah[6];
      ah[6] = ah[5];
      ah[5] = ah[4];
      ah[4] = ah[3] + temp1;
      ah[3] = ah[2];
      ah[2] = ah[1];
      ah[1] = ah[0];
      ah[0] = temp1 + temp2;
    }

    /* Add the compressed chunk to the current hash value: */
    for (i = 0; i < 8; i++)
      h[i] += ah[i];
  }

  /* Produce the final hash value (big-endian): */
  for (i = 0, j = 0; i < 8; i++)
  {
    hash[j++] = (uint8_t) (h[i] >> 24);
    hash[j++] = (uint8_t) (h[i] >> 16);
    hash[j++] = (uint8_t) (h[i] >> 8);
    hash[j++] = (uint8_t) h[i];
  }
}

int main() {

  freopen("./test.in", "r", stdin);

  char prev_digest_hex_str[65];
  uint64_t target;
  read_inputs(prev_digest_hex_str, &target);

  uint8_t input[INPUT_SIZE];
  uint32_t timestamp = 0x5bb16380;
  generate_partial_hash_input(input, timestamp, prev_digest_hex_str);

  uint8_t *d_input;
  cudaMalloc((void **) &d_input, sizeof(input));
  cudaMemcpy(d_input, input, sizeof(input), cudaMemcpyHostToDevice);

  uint64_t *d_target;
  cudaMalloc((void **) &d_target, sizeof(uint64_t));
  cudaMemcpy(d_target, &target, sizeof(target), cudaMemcpyHostToDevice);

  int *d_found;
  int found = 0;
  cudaMalloc((void **) &d_found, sizeof(int));
  cudaMemcpy(d_found, &found, sizeof(int), cudaMemcpyHostToDevice);

  uint8_t hash[32];
  uint8_t *d_hash;
  cudaMalloc((void **) &d_hash, sizeof(hash));

  uint64_t *d_nonce;
  uint64_t nonce[1] = {0};
  cudaMalloc((void **) &d_nonce, sizeof(nonce));

  find_nonce_kernel<<<10, 32>>>(d_input, d_target, d_found, d_hash, d_nonce);

  // Copy input back
  cudaMemcpy(input, d_input, sizeof(input), cudaMemcpyDeviceToHost);

  // Copy nonce back
  cudaMemcpy(nonce, d_nonce, sizeof(nonce), cudaMemcpyDeviceToHost);

  // Copy digest/hash back
  cudaMemcpy(hash, d_hash, sizeof(hash), cudaMemcpyDeviceToHost);

  // Print final output
  printf("Target: %llu\n", target);
  print_final_output(timestamp, nonce[0], hash);

  cudaFree(d_input); cudaFree(d_target); cudaFree(d_found);
  cudaFree(d_nonce); cudaFree(d_hash);

  return 0;
}

__global__ void find_nonce_kernel(uint8_t *g_input, uint64_t *g_target,
                                   /*volatile*/ int *found, uint8_t *hash,
                                   uint64_t *g_nonce) {
  // Copy from global to local
  uint8_t l_input[52];
  uint8_t l_hash[32];

  uint64_t l_target = *g_target;

  for (int i=0; i<NONCE_IDX; i++) l_input[i] = g_input[i];

  size_t total_num_threads = gridDim.x * gridDim.y * gridDim.z
                             * blockDim.x * blockDim.y * blockDim.z;

  size_t block_index_in_grid = blockIdx.x * (gridDim.y * gridDim.z)
                               + blockIdx.y * (gridDim.z) + blockIdx.z;

  size_t thread_index_in_block = threadIdx.x * (blockDim.y * blockDim.z)
                                 + threadIdx.y * (blockDim.z) + threadIdx.z;

  size_t thread_id = block_index_in_grid * (blockDim.x * blockDim.y * blockDim.z)
                     + thread_index_in_block;

  uint64_t nonce = thread_id;
//  uint64_t nonce = 0xe69d030000000000;

  // Start finding nonce
  while (!*found) {
    fill_input_with_nonce(l_input, nonce);
    sha256(l_hash, l_input, 52);

    if (*found == 0 && check_if_valid_nonce(l_hash, l_target)) {
      printf("FOUNDED\n");
      int old = atomicAdd(found, 1);
      if (old == 0) {
        // Only one thread can ever do this
        // Copy back input to global memory
        for (int i=0; i<INPUT_SIZE; i++) g_input[i] = l_input[i];

        // Copy hash to global memory
        for (int i=0; i<32; i++) hash[i] = l_hash[i];

        // Copy over nonce
        *g_nonce = nonce;
      }
    }
    nonce += total_num_threads;
  }

}