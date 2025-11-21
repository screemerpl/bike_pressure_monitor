/*
  FastLZ - Byte-aligned LZ77 compression library
  Copyright (C) 2005-2020 Ariya Hidayat <ariya.hidayat@gmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#include "ui.h"
#include "fastlz.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <stdint.h>

#if defined(__GNUC__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif

/*
 * Give hints to the compiler for branch prediction optimization.
 */
#if defined(__clang__) || (defined(__GNUC__) && (__GNUC__ > 2))
#define FASTLZ_LIKELY(c) (__builtin_expect(!!(c), 1))
#define FASTLZ_UNLIKELY(c) (__builtin_expect(!!(c), 0))
#else
#define FASTLZ_LIKELY(c) (c)
#define FASTLZ_UNLIKELY(c) (c)
#endif

/*
 * Specialize custom 64-bit implementation for speed improvements.
 */
#if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__)
#define FLZ_ARCH64
#endif

/*
 * Workaround for DJGPP to find uint8_t, uint16_t, etc.
 */
#if defined(__MSDOS__) && defined(__GNUC__)
#include <stdint-gcc.h>
#endif

static int fastlz_memmove(uint8_t* dest, const uint8_t* src, uint32_t count) {
  /* Validate input parameters */
  if (dest == NULL) {
    printf("fastlz_memmove: ERROR - dest pointer is NULL\n");
    return -1;
  }
  
  if (src == NULL) {
    printf("fastlz_memmove: ERROR - src pointer is NULL (dest=%p count=%lu)\n", dest, count);
    return -1;
  }
  
  if (count == 0) {
    printf("fastlz_memmove: WARNING - count is 0 (dest=%p src=%p)\n", dest, src);
    return 0;   /* Success: nothing to copy */
  }
  
  /* Perform safe byte-by-byte copy */
  for (uint32_t i = 0; i < count; i++) {
    dest[i] = src[i];
  }
  
  return 0;
}

static int fastlz_memcpy(uint8_t* dest, const uint8_t* src, uint32_t count) {
  return fastlz_memmove(dest, src, count);
}

#define MAX_L1_DISTANCE 8192
#define MAX_L2_DISTANCE 8191

#define FASTLZ_BOUND_CHECK(cond) \
  if (FASTLZ_UNLIKELY(!(cond))) return 0;


int fastlz1_decompress(const void* input, int length, void* output, int maxout) {
  const uint8_t* ip = (const uint8_t*)input;
  const uint8_t* ip_limit = ip + length;
  const uint8_t* ip_bound = ip_limit - 2;
  uint8_t* op = (uint8_t*)output;
  uint8_t* op_limit = op + maxout;
  
  /* Validate main pointers at start */
  if (input == NULL) {
    printf("fastlz1_decompress: ERROR - input is NULL\n");
    return 0;
  }
  if (output == NULL) {
    printf("fastlz1_decompress: ERROR - output is NULL (input=%p length=%d maxout=%d)\n", 
           input, length, maxout);
    return 0;
  }
  if (length < 1) {
    printf("fastlz1_decompress: ERROR - invalid input length: %d\n", length);
    return 0;
  }
  if (maxout < 1) {
    printf("fastlz1_decompress: ERROR - invalid maxout: %d\n", maxout);
    return 0;
  }
  
  uint32_t ctrl = (*ip++) & 31;

  while (1) {
    if (ctrl >= 32) {
      uint32_t len = (ctrl >> 5) - 1;
      uint32_t ofs = (ctrl & 31) << 8;
      const uint8_t* ref = op - ofs - 1;
      if (len == 7 - 1) {
        FASTLZ_BOUND_CHECK(ip <= ip_bound);
        len += *ip++;
      }
      ref -= *ip++;
      len += 3;
      FASTLZ_BOUND_CHECK(op + len <= op_limit);
      FASTLZ_BOUND_CHECK(ref >= (uint8_t*)output);
      
      /* Extra validation before memmove */
      if (op == NULL) {
        printf("fastlz1_decompress: ERROR - op pointer is NULL (ref=%p len=%lu)\n", ref, len);
        return 0;
      }
      if (ref == NULL) {
        printf("fastlz1_decompress: ERROR - ref pointer is NULL (op=%p len=%lu)\n", op, len);
        return 0;
      }
      
      if (fastlz_memmove(op, ref, len) < 0) {
        printf("fastlz1_decompress: ERROR - fastlz_memmove failed at op=%p ref=%p len=%lu\n", op, ref, len);
        return 0;
      }
      op += len;
    } else {
      ctrl++;
      FASTLZ_BOUND_CHECK(op + ctrl <= op_limit);
      FASTLZ_BOUND_CHECK(ip + ctrl <= ip_limit);
      
      /* Extra validation before memcpy */
      if (op == NULL) {
        printf("fastlz1_decompress: ERROR - op pointer is NULL (ctrl=%lu)\n", ctrl);
        return 0;
      }
      if (ip == NULL) {
        printf("fastlz1_decompress: ERROR - ip pointer is NULL (ctrl=%lu)\n", ctrl);
        return 0;
      }
      
      if (fastlz_memcpy(op, ip, ctrl) < 0) {
        printf("fastlz1_decompress: ERROR - fastlz_memcpy failed at op=%p ip=%p ctrl=%lu\n", op, ip, ctrl);
        return 0;
      }
      ip += ctrl;
      op += ctrl;
    }

    if (FASTLZ_UNLIKELY(ip > ip_bound)) break;
    ctrl = *ip++;
  }

  return (int)(op - (uint8_t*)output);
}


int fastlz2_decompress(const void* input, int length, void* output, int maxout) {
  const uint8_t* ip = (const uint8_t*)input;
  const uint8_t* ip_limit = ip + length;
  const uint8_t* ip_bound = ip_limit - 2;
  uint8_t* op = (uint8_t*)output;
  uint8_t* op_limit = op + maxout;
  
  /* Validate main pointers at start */
  if (input == NULL) {
    printf("fastlz2_decompress: ERROR - input is NULL\n");
    return 0;
  }
  if (output == NULL) {
    printf("fastlz2_decompress: ERROR - output is NULL (input=%p length=%d maxout=%d)\n", 
           input, length, maxout);
    return 0;
  }
  if (length < 1) {
    printf("fastlz2_decompress: ERROR - invalid input length: %d\n", length);
    return 0;
  }
  if (maxout < 1) {
    printf("fastlz2_decompress: ERROR - invalid maxout: %d\n", maxout);
    return 0;
  }
  
  uint32_t ctrl = (*ip++) & 31;

  while (1) {
    if (ctrl >= 32) {
      uint32_t len = (ctrl >> 5) - 1;
      uint32_t ofs = (ctrl & 31) << 8;
      const uint8_t* ref = op - ofs - 1;

      uint8_t code;
      if (len == 7 - 1) do {
          FASTLZ_BOUND_CHECK(ip <= ip_bound);
          code = *ip++;
          len += code;
        } while (code == 255);
      code = *ip++;
      ref -= code;
      len += 3;

      /* match from 16-bit distance */
      if (FASTLZ_UNLIKELY(code == 255))
        if (FASTLZ_LIKELY(ofs == (31 << 8))) {
          FASTLZ_BOUND_CHECK(ip < ip_bound);
          ofs = (*ip++) << 8;
          ofs += *ip++;
          ref = op - ofs - MAX_L2_DISTANCE - 1;
        }

      FASTLZ_BOUND_CHECK(op + len <= op_limit);
      FASTLZ_BOUND_CHECK(ref >= (uint8_t*)output);
      
      /* Extra validation before memmove */
      if (op == NULL) {
        printf("fastlz2_decompress: ERROR - op pointer is NULL (ref=%p len=%lu)\n", ref, len);
        return 0;
      }
      if (ref == NULL) {
        printf("fastlz2_decompress: ERROR - ref pointer is NULL (op=%p len=%lu)\n", op, len);
        return 0;
      }
      
      if (fastlz_memmove(op, ref, len) < 0) {
        printf("fastlz2_decompress: ERROR - fastlz_memmove failed at op=%p ref=%p len=%lu\n", op, ref, len);
        return 0;
      }
      op += len;
    } else {
      ctrl++;
      FASTLZ_BOUND_CHECK(op + ctrl <= op_limit);
      FASTLZ_BOUND_CHECK(ip + ctrl <= ip_limit);
      
      /* Extra validation before memcpy */
      if (op == NULL) {
        printf("fastlz2_decompress: ERROR - op pointer is NULL (ctrl=%lu)\n", ctrl);
        return 0;
      }
      if (ip == NULL) {
        printf("fastlz2_decompress: ERROR - ip pointer is NULL (ctrl=%lu)\n", ctrl);
        return 0;
      }
      
      if (fastlz_memcpy(op, ip, ctrl) < 0) {
        printf("fastlz2_decompress: ERROR - fastlz_memcpy failed at op=%p ip=%p ctrl=%lu\n", op, ip, ctrl);
        return 0;
      }
      ip += ctrl;
      op += ctrl;
    }

    if (FASTLZ_UNLIKELY(ip >= ip_limit)) break;
    ctrl = *ip++;
  }

  return (int)(op - (uint8_t*)output);
}

int fastlz_decompress(const void* input, int length, void* output, int maxout) {
  /* magic identifier for compression level */
  if (input == NULL) {
    printf("fastlz_decompress: ERROR - input pointer is NULL\n");
    return 0;
  }
  if (length < 1) {
    printf("fastlz_decompress: ERROR - invalid length: %d (input=%p output=%p)\n", 
           length, input, output);
    return 0;
  }
  if (maxout < 1) {
    printf("fastlz_decompress: ERROR - invalid maxout: %d (input=%p output=%p length=%d)\n", 
           maxout, input, output, length);
    return 0;
  }
  
  /* Handle NULL output buffer by allocating memory */
  void* allocated_buffer = NULL;
  if (output == NULL) {
    printf("fastlz_decompress: WARNING - output pointer is NULL, allocating buffer (length=%d maxout=%d)\n", 
           length, maxout);
    allocated_buffer = malloc(maxout);
    if (allocated_buffer == NULL) {
      printf("fastlz_decompress: ERROR - failed to allocate %d bytes for decompression\n", maxout);
      return 0;
    }
    output = allocated_buffer;
  }
  
  int level = ((*(const uint8_t*)input) >> 5) + 1;
  printf("fastlz_decompress: Decompression level %d (input=%p output=%p length=%d maxout=%d)%s\n", 
         level, input, output, length, maxout,
         allocated_buffer ? " [ALLOCATED]" : "");

  int result;
  if (level == 1) result = fastlz1_decompress(input, length, output, maxout);
  else if (level == 2) result = fastlz2_decompress(input, length, output, maxout);
  else {
    printf("fastlz_decompress: ERROR - unknown compression level: %d\n", level);
    result = 0;
  }
  
  /* Free allocated buffer after decompression */
  if (allocated_buffer != NULL) {
    free(allocated_buffer);
    allocated_buffer = NULL;
  }
  
  return result;
}


#if defined(__GNUC__)
    #pragma GCC diagnostic pop
#endif