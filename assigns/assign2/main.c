#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define NUM_BITS 256

int main() {

  // Both digest and target (compared against hash) are 256 bits.
  uint8_t digest[32], target[32];
  memset(digest, sizeof(digest), 0);
  memset(target, sizeof(target), 0);

  // 1 hex digit is 4 bits, 2 hex digit is 8 bits (uint8_t)
  char digest_hex_str[65], target_dec_str[65];
  scanf("%s", digest_hex_str);
  scanf("%s", target_dec_str);

  // Convert digest str to uint8_t arr
  char mini_digest[3];
  for (int i=0; i<64; i+=2) {
    strncpy(mini_digest, digest_hex_str+i, 2);
    digest[i/2] = (uint8_t) strtol(mini_digest, NULL, 16);
  }
  
  return 0;
}