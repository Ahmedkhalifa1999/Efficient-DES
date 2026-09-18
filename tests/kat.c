/* kat.c - unit tests for the DES core, independent of the CLI.
 *
 * 1. Known-answer tests (FIPS 46 / textbook vectors).
 * 2. The expansion-by-rotation identity used in feistel() against the E table.
 * 3. Random keys and blocks against an independent bit-loop reference DES,
 *    and decrypt(encrypt(x)) == x.
 */
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "des.h"

static int failures;

#define CHECK(cond, ...)                                                                           \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            failures++;                                                                            \
            fprintf(stderr, "FAIL %s:%d: ", __FILE__, __LINE__);                                   \
            fprintf(stderr, __VA_ARGS__);                                                          \
            fputc('\n', stderr);                                                                   \
        }                                                                                          \
    } while (0)

/* ---- reference implementation: straight from the FIPS tables ---------- */

static const uint8_t R_IP[64] = {58, 50, 42, 34, 26, 18, 10, 2, 60, 52, 44, 36, 28, 20, 12, 4,
                                 62, 54, 46, 38, 30, 22, 14, 6, 64, 56, 48, 40, 32, 24, 16, 8,
                                 57, 49, 41, 33, 25, 17, 9,  1, 59, 51, 43, 35, 27, 19, 11, 3,
                                 61, 53, 45, 37, 29, 21, 13, 5, 63, 55, 47, 39, 31, 23, 15, 7};
static const uint8_t R_FP[64] = {40, 8, 48, 16, 56, 24, 64, 32, 39, 7, 47, 15, 55, 23, 63, 31,
                                 38, 6, 46, 14, 54, 22, 62, 30, 37, 5, 45, 13, 53, 21, 61, 29,
                                 36, 4, 44, 12, 52, 20, 60, 28, 35, 3, 43, 11, 51, 19, 59, 27,
                                 34, 2, 42, 10, 50, 18, 58, 26, 33, 1, 41, 9,  49, 17, 57, 25};
static const uint8_t R_E[48] = {32, 1,  2,  3,  4,  5,  4,  5,  6,  7,  8,  9,  8,  9,  10, 11,
                                12, 13, 12, 13, 14, 15, 16, 17, 16, 17, 18, 19, 20, 21, 20, 21,
                                22, 23, 24, 25, 24, 25, 26, 27, 28, 29, 28, 29, 30, 31, 32, 1};
static const uint8_t R_P[32] = {16, 7, 20, 21, 29, 12, 28, 17, 1,  15, 23, 26, 5,  18, 31, 10,
                                2,  8, 24, 14, 32, 27, 3,  9,  19, 13, 30, 6,  22, 11, 4,  25};
static const uint8_t R_PC1[56] = {57, 49, 41, 33, 25, 17, 9,  1,  58, 50, 42, 34, 26, 18,
                                  10, 2,  59, 51, 43, 35, 27, 19, 11, 3,  60, 52, 44, 36,
                                  63, 55, 47, 39, 31, 23, 15, 7,  62, 54, 46, 38, 30, 22,
                                  14, 6,  61, 53, 45, 37, 29, 21, 13, 5,  28, 20, 12, 4};
static const uint8_t R_PC2[48] = {14, 17, 11, 24, 1,  5,  3,  28, 15, 6,  21, 10, 23, 19, 12, 4,
                                  26, 8,  16, 7,  27, 20, 13, 2,  41, 52, 31, 37, 47, 55, 30, 40,
                                  51, 45, 33, 48, 44, 49, 39, 56, 34, 53, 46, 42, 50, 36, 29, 32};
static const uint8_t R_SHIFTS[16] = {1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1};
static const uint8_t R_S[8][64] = {
    {14, 4,  13, 1, 2,  15, 11, 8, 3, 10, 6, 12, 5,  9,  0,  7,  0,  15, 7,  4,  14, 2,
     13, 1,  10, 6, 12, 11, 9,  5, 3, 8,  4, 1,  14, 8,  13, 6,  2,  11, 15, 12, 9,  7,
     3,  10, 5,  0, 15, 12, 8,  2, 4, 9,  1, 7,  5,  11, 3,  14, 10, 0,  6,  13},
    {15, 1,  8,  14, 6,  11, 3,  4, 9,  7,  2, 13, 12, 0,  5,  10, 3,  13, 4,  7, 15, 2,
     8,  14, 12, 0,  1,  10, 6,  9, 11, 5,  0, 14, 7,  11, 10, 4,  13, 1,  5,  8, 12, 6,
     9,  3,  2,  15, 13, 8,  10, 1, 3,  15, 4, 2,  11, 6,  7,  12, 0,  5,  14, 9},
    {10, 0,  9,  14, 6, 3,  15, 5,  1,  13, 12, 7, 11, 4,  2,  8,  13, 7, 0,  9, 3, 4,
     6,  10, 2,  8,  5, 14, 12, 11, 15, 1,  13, 6, 4,  9,  8,  15, 3,  0, 11, 1, 2, 12,
     5,  10, 14, 7,  1, 10, 13, 0,  6,  9,  8,  7, 4,  15, 14, 3,  11, 5, 2,  12},
    {7, 13, 14, 3, 0, 6,  9, 10, 1,  2, 8,  5, 11, 12, 4,  15, 13, 8,  11, 5, 6, 15,
     0, 3,  4,  7, 2, 12, 1, 10, 14, 9, 10, 6, 9,  0,  12, 11, 7,  13, 15, 1, 3, 14,
     5, 2,  8,  4, 3, 15, 0, 6,  10, 1, 13, 8, 9,  4,  5,  11, 12, 7,  2,  14},
    {2,  12, 4, 1,  7,  10, 11, 6, 8, 5,  3, 15, 13, 0,  14, 9,  14, 11, 2,  12, 4,  7,
     13, 1,  5, 0,  15, 10, 3,  9, 8, 6,  4, 2,  1,  11, 10, 13, 7,  8,  15, 9,  12, 5,
     6,  3,  0, 14, 11, 8,  12, 7, 1, 14, 2, 13, 6,  15, 0,  9,  10, 4,  5,  3},
    {12, 1,  10, 15, 9,  2,  6, 8,  0, 13, 3,  4,  14, 7,  5, 11, 10, 15, 4, 2, 7, 12,
     9,  5,  6,  1,  13, 14, 0, 11, 3, 8,  9,  14, 15, 5,  2, 8,  12, 3,  7, 0, 4, 10,
     1,  13, 11, 6,  4,  3,  2, 12, 9, 5,  15, 10, 11, 14, 1, 7,  6,  0,  8, 13},
    {4, 11, 2,  14, 15, 0,  8,  13, 3, 12, 9,  7, 5,  10, 6,  1,  13, 0,  11, 7,  4, 9,
     1, 10, 14, 3,  5,  12, 2,  15, 8, 6,  1,  4, 11, 13, 12, 3,  7,  14, 10, 15, 6, 8,
     0, 5,  9,  2,  6,  11, 13, 8,  1, 4,  10, 7, 9,  5,  0,  15, 14, 2,  3,  12},
    {13, 2, 8,  4, 6, 15, 11, 1,  10, 9,  3, 14, 5,  0,  12, 7,  1,  15, 13, 8, 10, 3,
     7,  4, 12, 5, 6, 11, 0,  14, 9,  2,  7, 11, 4,  1,  9,  12, 14, 2,  0,  6, 10, 13,
     15, 3, 5,  8, 2, 1,  14, 7,  4,  10, 8, 13, 15, 12, 9,  0,  3,  5,  6,  11},
};

static uint64_t ref_permute(uint64_t x, unsigned in_w, const uint8_t *t, unsigned out_w) {
    uint64_t out = 0;
    for (unsigned i = 0; i < out_w; i++) {
        out |= ((x >> (in_w - t[i])) & 1u) << (out_w - 1 - i);
    }
    return out;
}

static uint32_t ref_f(uint32_t r, uint64_t k48) {
    uint64_t e = ref_permute(r, 32, R_E, 48) ^ k48;
    uint32_t s = 0;
    for (unsigned b = 0; b < 8; b++) {
        unsigned v = (unsigned)((e >> (42 - 6 * b)) & 0x3F);
        unsigned row = ((v >> 4) & 2u) | (v & 1u);
        unsigned col = (v >> 1) & 15u;
        s = (s << 4) | R_S[b][row * 16 + col];
    }
    return (uint32_t)ref_permute(s, 32, R_P, 32);
}

static uint64_t ref_des(uint64_t key, uint64_t block, int decrypt) {
    uint64_t subkeys[16];
    uint64_t cd = ref_permute(key, 64, R_PC1, 56);
    uint32_t c = (uint32_t)(cd >> 28) & 0x0FFFFFFFu;
    uint32_t d = (uint32_t)cd & 0x0FFFFFFFu;
    uint64_t x;
    uint32_t l;
    uint32_t r;

    for (unsigned i = 0; i < 16; i++) {
        unsigned s = R_SHIFTS[i];
        c = ((c << s) | (c >> (28 - s))) & 0x0FFFFFFFu;
        d = ((d << s) | (d >> (28 - s))) & 0x0FFFFFFFu;
        subkeys[i] = ref_permute(((uint64_t)c << 28) | d, 56, R_PC2, 48);
    }
    x = ref_permute(block, 64, R_IP, 64);
    l = (uint32_t)(x >> 32);
    r = (uint32_t)x;
    for (unsigned i = 0; i < 16; i++) {
        uint32_t t = l ^ ref_f(r, subkeys[decrypt ? 15 - i : i]);
        l = r;
        r = t;
    }
    return ref_permute(((uint64_t)r << 32) | l, 64, R_FP, 64);
}

/* xorshift64*: deterministic pseudo-random values for the tests. */
static uint64_t rng_state = 0x9E3779B97F4A7C15ull;
static uint64_t rng(void) {
    rng_state ^= rng_state >> 12;
    rng_state ^= rng_state << 25;
    rng_state ^= rng_state >> 27;
    return rng_state * 0x2545F4914F6CDD1Dull;
}

static uint32_t rotl32(uint32_t x, unsigned n) {
    return (x << n) | (x >> (32 - n));
}

int main(void) {
    static const struct {
        uint64_t key, plain, cipher;
    } kat[] = {
        {0x133457799BBCDFF1ull, 0x0123456789ABCDEFull, 0x85E813540F0AB405ull},
        {0x0000000000000000ull, 0x0000000000000000ull, 0x8CA64DE9C1B123A7ull},
        {0x0123456789ABCDEFull, 0x4E6F772069732074ull, 0x3FA40E8A984D4815ull},
        {0xFFFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull, 0x7359B2163E4EDC58ull},
        {0x0E329232EA6D0D73ull, 0x8787878787878787ull, 0x0000000000000000ull},
    };
    des_key_schedule ks;

    for (size_t i = 0; i < sizeof kat / sizeof kat[0]; i++) {
        uint64_t c;
        uint64_t p;
        des_set_key(&ks, kat[i].key);
        c = des_encrypt_block(&ks, kat[i].plain);
        p = des_decrypt_block(&ks, kat[i].cipher);
        CHECK(c == kat[i].cipher, "KAT %zu encrypt: got %016" PRIX64 " want %016" PRIX64, i, c,
              kat[i].cipher);
        CHECK(p == kat[i].plain, "KAT %zu decrypt: got %016" PRIX64 " want %016" PRIX64, i, p,
              kat[i].plain);
    }

    for (int n = 0; n < 100000; n++) {
        uint32_t r = (uint32_t)rng();
        uint64_t e = ref_permute(r, 32, R_E, 48);
        for (unsigned b = 0; b < 8; b++) {
            uint32_t want = (uint32_t)((e >> (42 - 6 * b)) & 0x3F);
            uint32_t got = rotl32(r, (5 + 4 * b) % 32) & 0x3F;
            CHECK(got == want, "E identity: R=%08" PRIX32 " box %u got %u want %u", r, b, got,
                  want);
            if (failures > 10) {
                return 1;
            }
        }
    }

    for (int n = 0; n < 20000; n++) {
        uint64_t key = rng();
        uint64_t p = rng();
        uint64_t c;
        uint64_t want = ref_des(key, p, 0);
        des_set_key(&ks, key);
        c = des_encrypt_block(&ks, p);
        CHECK(c == want, "random encrypt: key=%016" PRIX64 " p=%016" PRIX64, key, p);
        CHECK(des_decrypt_block(&ks, c) == p, "round trip: key=%016" PRIX64 " p=%016" PRIX64, key,
              p);
        CHECK(ref_des(key, c, 1) == p, "reference decrypt: key=%016" PRIX64, key);
        if (failures > 10) {
            return 1;
        }
    }

    CHECK(des_is_weak_key(0x0101010101010101ull), "weak key not detected");
    CHECK(des_is_weak_key(0x0000000000000000ull), "weak key (parity stripped) not detected");
    CHECK(des_is_weak_key(0x01FE01FE01FE01FEull), "semi-weak key not detected");
    CHECK(!des_is_weak_key(0x133457799BBCDFF1ull), "normal key flagged as weak");

    {
        uint8_t bytes[8] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
        uint8_t out[8];
        CHECK(des_load_be64(bytes) == 0x0123456789ABCDEFull, "load_be64");
        des_store_be64(out, 0x0123456789ABCDEFull);
        CHECK(out[0] == 0x01 && out[7] == 0xEF, "store_be64");
    }

    if (failures == 0) {
        puts("kat: all tests passed");
        return 0;
    }
    fprintf(stderr, "kat: %d failure(s)\n", failures);
    return 1;
}
