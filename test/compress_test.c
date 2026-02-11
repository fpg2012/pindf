#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../pindf.h"

#define TEST_DATA "This is a test string for compression. " \
                  "It should be long enough to demonstrate " \
                  "that compression actually works. " \
                  "Repeating patterns help compression ratio. " \
                  "This is a test string for compression. " \
                  "It should be long enough to demonstrate " \
                  "that compression actually works. " \
                  "Repeating patterns help compression ratio."

// Helper function to compare two byte strings
static int compare_uchar_str(const pindf_uchar_str *a, const pindf_uchar_str *b) {
    if (a->len != b->len) return 0;
    return memcmp(a->p, b->p, a->len) == 0;
}

// Test basic compression and decompression
static void test_basic_roundtrip(void) {
    printf("Testing basic roundtrip compression/decompression...\n");

    // Prepare source data
    size_t src_len = strlen(TEST_DATA);
    pindf_uchar_str src;
    pindf_uchar_str_init(&src, src_len);
    memcpy(src.p, TEST_DATA, src_len);

    // Allocate buffers for compressed and decompressed data
    // Start with reasonable size
    pindf_uchar_str compressed;
    pindf_uchar_str_init(&compressed, src_len); // Initial size same as source

    pindf_uchar_str decompressed;
    pindf_uchar_str_init(&decompressed, src_len); // Initial size same as source

    // Compress
    int compressed_size = pindf_zlib_compress(&compressed, &src);
    printf("  Original size: %zu, Compressed size: %d\n", src.len, compressed_size);

    assert(compressed_size > 0 && "Compression should succeed");
    assert(compressed_size <= (int)src.len && "Compressed data should not be larger than original");

    // Decompress
    pindf_uchar_str_init(&decompressed, src.len); // Reset size
    int decompressed_size = pindf_zlib_uncompress(&decompressed, &compressed);

    assert(decompressed_size == (int)src.len && "Decompressed size should match original");
    assert(compare_uchar_str(&src, &decompressed) && "Decompressed data should match original");

    printf("  Roundtrip test passed.\n");

    // Cleanup
    pindf_uchar_str_destroy(&src);
    pindf_uchar_str_destroy(&compressed);
    pindf_uchar_str_destroy(&decompressed);
}

// Test with small buffer to trigger expansion
static void test_buffer_expansion(void) {
    printf("Testing buffer expansion...\n");

    // Prepare source data
    size_t src_len = strlen(TEST_DATA);
    pindf_uchar_str src;
    pindf_uchar_str_init(&src, src_len);
    memcpy(src.p, TEST_DATA, src_len);

    // Intentionally use small buffer (but not too small to avoid hitting retry limit)
    // We need at least src_len/2^4 = 309/16 ≈ 20 bytes to stay within PINDF_MAX_EXPAND_RETRY=4
    pindf_uchar_str compressed;
    pindf_uchar_str_init(&compressed, 50); // Small but reasonable buffer

    // Compression should expand buffer if needed
    int compressed_size = pindf_zlib_compress(&compressed, &src);
    assert(compressed_size > 0 && "Compression should succeed with buffer expansion");
    assert(compressed.capacity >= (size_t)compressed_size && "Buffer should be large enough");

    printf("  Initial buffer: 50, Final buffer: %zu, Compressed size: %d\n",
           compressed.capacity, compressed_size);

    // Decompress with small buffer (but large enough to stay within retry limit)
    // Decompressed size is src_len (309). With initial buffer 50, we need to expand:
    // 50→100→200→400 (3 expansions, within PINDF_MAX_EXPAND_RETRY=4)
    pindf_uchar_str decompressed;
    pindf_uchar_str_init(&decompressed, 50); // Small buffer

    int decompressed_size = pindf_zlib_uncompress(&decompressed, &compressed);
    assert(decompressed_size == (int)src.len && "Decompression should succeed with buffer expansion");
    assert(compare_uchar_str(&src, &decompressed) && "Decompressed data should match original");

    printf("  Buffer expansion test passed.\n");

    pindf_uchar_str_destroy(&src);
    pindf_uchar_str_destroy(&compressed);
    pindf_uchar_str_destroy(&decompressed);
}

// Test with empty data
static void test_empty_data(void) {
    printf("Testing empty data...\n");

    pindf_uchar_str src;
    pindf_uchar_str_init(&src, 0); // Empty data

    pindf_uchar_str compressed;
    pindf_uchar_str_init(&compressed, 100); // Enough space

    int compressed_size = pindf_zlib_compress(&compressed, &src);
    assert(compressed_size > 0 && "Empty data compression should succeed");

    pindf_uchar_str decompressed;
    pindf_uchar_str_init(&decompressed, 100);

    int decompressed_size = pindf_zlib_uncompress(&decompressed, &compressed);
    assert(decompressed_size == 0 && "Decompressed empty data should have size 0");
    assert(decompressed.len == 0 && "Decompressed buffer length should be 0");

    printf("  Empty data test passed.\n");

    pindf_uchar_str_destroy(&src);
    pindf_uchar_str_destroy(&compressed);
    pindf_uchar_str_destroy(&decompressed);
}

// Test with random binary data
static void test_binary_data(void) {
    printf("Testing random binary data...\n");

    // Generate some binary data
    size_t src_len = 1024;
    pindf_uchar_str src;
    pindf_uchar_str_init(&src, src_len);

    // Fill with pseudo-random pattern
    for (size_t i = 0; i < src_len; i++) {
        src.p[i] = (uchar)(i * 31 + 17) % 256;
    }

    pindf_uchar_str compressed;
    pindf_uchar_str_init(&compressed, src_len);

    pindf_uchar_str decompressed;
    pindf_uchar_str_init(&decompressed, src_len);

    // Compress
    int compressed_size = pindf_zlib_compress(&compressed, &src);
    assert(compressed_size > 0 && "Binary data compression should succeed");

    // Decompress
    int decompressed_size = pindf_zlib_uncompress(&decompressed, &compressed);
    assert(decompressed_size == (int)src.len && "Decompressed size should match original");
    assert(compare_uchar_str(&src, &decompressed) && "Decompressed data should match original");

    printf("  Binary data test passed (compression ratio: %.2f%%).\n",
           (float)compressed_size / src_len * 100.0f);

    pindf_uchar_str_destroy(&src);
    pindf_uchar_str_destroy(&compressed);
    pindf_uchar_str_destroy(&decompressed);
}

// Test error handling (invalid compressed data)
static void test_error_handling(void) {
    printf("Testing error handling...\n");

    // Create invalid compressed data
    pindf_uchar_str invalid_data;
    pindf_uchar_str_init(&invalid_data, 20);
    for (size_t i = 0; i < 20; i++) {
        invalid_data.p[i] = (uchar)(i * 7); // Not valid zlib data
    }

    pindf_uchar_str dest;
    pindf_uchar_str_init(&dest, 100);

    // Attempt to decompress invalid data
    int result = pindf_zlib_uncompress(&dest, &invalid_data);
    assert(result == PINDF_FLTR_DAT_ERR && "Invalid data should return DATA_ERROR");

    printf("  Error handling test passed.\n");

    pindf_uchar_str_destroy(&invalid_data);
    pindf_uchar_str_destroy(&dest);
}

// Test with exact buffer size (no expansion needed)
static void test_exact_buffer_size(void) {
    printf("Testing exact buffer size...\n");

    // Prepare source data
    size_t src_len = strlen(TEST_DATA);
    pindf_uchar_str src;
    pindf_uchar_str_init(&src, src_len);
    memcpy(src.p, TEST_DATA, src_len);

    // First, compress to find out compressed size
    pindf_uchar_str temp_compressed;
    pindf_uchar_str_init(&temp_compressed, src_len);
    int compressed_size = pindf_zlib_compress(&temp_compressed, &src);
    assert(compressed_size > 0);

    // Now allocate buffer of exact size
    pindf_uchar_str compressed_exact;
    pindf_uchar_str_init(&compressed_exact, compressed_size);

    // Compress again - should fit exactly
    int compressed_size2 = pindf_zlib_compress(&compressed_exact, &src);
    assert(compressed_size2 == compressed_size && "Compression with exact buffer should work");
    assert(compressed_exact.capacity == (size_t)compressed_size && "Buffer should be exact size");

    // Decompress with exact buffer size
    pindf_uchar_str decompressed_exact;
    pindf_uchar_str_init(&decompressed_exact, src_len);

    int decompressed_size = pindf_zlib_uncompress(&decompressed_exact, &compressed_exact);
    assert(decompressed_size == (int)src.len && "Decompression with exact buffer should work");
    assert(compare_uchar_str(&src, &decompressed_exact) && "Decompressed data should match original");

    printf("  Exact buffer size test passed.\n");

    pindf_uchar_str_destroy(&src);
    pindf_uchar_str_destroy(&temp_compressed);
    pindf_uchar_str_destroy(&compressed_exact);
    pindf_uchar_str_destroy(&decompressed_exact);
}

// Test the pindf_flate_encode and pindf_flate_decode functions
static void test_flate_filter(void) {
    printf("Testing FlateDecode filter...\n");

    // Prepare source data
    size_t src_len = strlen(TEST_DATA);
    pindf_uchar_str src;
    pindf_uchar_str_init(&src, src_len);
    memcpy(src.p, TEST_DATA, src_len);

    // Create a filter
    pindf_stream_filter filter;
    int init_result = pindf_filter_init(&filter, PINDF_FLTR_TYPE_FLATEDECODE, NULL);
    assert(init_result == 0 && "Filter initialization should succeed");

    // Allocate buffers
    pindf_uchar_str encoded;
    pindf_uchar_str_init(&encoded, src_len);

    pindf_uchar_str decoded;
    pindf_uchar_str_init(&decoded, src_len);

    // Encode (compress)
    int encode_result = pindf_flate_encode(&filter, &encoded, &src);
    assert(encode_result > 0 && "Flate encode should succeed");

    // Decode (decompress)
    int decode_result = pindf_flate_decode(&filter, &decoded, &encoded);
    assert(decode_result == (int)src.len && "Flate decode should return original size");
    assert(compare_uchar_str(&src, &decoded) && "Decoded data should match original");

    printf("  Flate filter test passed.\n");

    pindf_uchar_str_destroy(&src);
    pindf_uchar_str_destroy(&encoded);
    pindf_uchar_str_destroy(&decoded);
}

int main(void) {
    printf("=== Starting compress/decompress tests ===\n\n");

    test_basic_roundtrip();
    printf("\n");

    test_buffer_expansion();
    printf("\n");

    test_empty_data();
    printf("\n");

    test_binary_data();
    printf("\n");

    test_error_handling();
    printf("\n");

    test_exact_buffer_size();
    printf("\n");

    test_flate_filter();
    printf("\n");

    printf("=== All tests passed! ===\n");
    return 0;
}