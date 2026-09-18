/* des.c - DES block cipher, table-driven.
 *
 * The standard tables (S-boxes, IP, IP^-1, E, P, PC-1, PC-2) are kept in
 * their textbook form. At start-up they are turned into the lookup tables
 * that the hot path uses:
 *
 *   SP[b][v]  = P(S_b(v) placed in its 4-bit slot)   -> one round is eight
 *               table lookups XORed together; the P permutation is folded in.
 *   IPT[k][x] = IP(byte x placed in byte slot k)      -> IP is eight lookups
 *   FPT[k][x] = IP^-1(byte x placed in byte slot k)   -> same for IP^-1
 *
 * The expansion E is not a table: for the right half R, the 6-bit input of
 * S-box b is  rotl32(R, 5 + 4*b) & 0x3F  (verified in tests/kat.c).
 */
#include "des.h"

/* ---- standard DES tables (bit 1 = MSB) --------------------------------- */

static const uint8_t S[8][4][16] = {
    {{14, 4, 13, 1, 2, 15, 11, 8, 3, 10, 6, 12, 5, 9, 0, 7},
     {0, 15, 7, 4, 14, 2, 13, 1, 10, 6, 12, 11, 9, 5, 3, 8},
     {4, 1, 14, 8, 13, 6, 2, 11, 15, 12, 9, 7, 3, 10, 5, 0},
     {15, 12, 8, 2, 4, 9, 1, 7, 5, 11, 3, 14, 10, 0, 6, 13}},
    {{15, 1, 8, 14, 6, 11, 3, 4, 9, 7, 2, 13, 12, 0, 5, 10},
     {3, 13, 4, 7, 15, 2, 8, 14, 12, 0, 1, 10, 6, 9, 11, 5},
     {0, 14, 7, 11, 10, 4, 13, 1, 5, 8, 12, 6, 9, 3, 2, 15},
     {13, 8, 10, 1, 3, 15, 4, 2, 11, 6, 7, 12, 0, 5, 14, 9}},
    {{10, 0, 9, 14, 6, 3, 15, 5, 1, 13, 12, 7, 11, 4, 2, 8},
     {13, 7, 0, 9, 3, 4, 6, 10, 2, 8, 5, 14, 12, 11, 15, 1},
     {13, 6, 4, 9, 8, 15, 3, 0, 11, 1, 2, 12, 5, 10, 14, 7},
     {1, 10, 13, 0, 6, 9, 8, 7, 4, 15, 14, 3, 11, 5, 2, 12}},
    {{7, 13, 14, 3, 0, 6, 9, 10, 1, 2, 8, 5, 11, 12, 4, 15},
     {13, 8, 11, 5, 6, 15, 0, 3, 4, 7, 2, 12, 1, 10, 14, 9},
     {10, 6, 9, 0, 12, 11, 7, 13, 15, 1, 3, 14, 5, 2, 8, 4},
     {3, 15, 0, 6, 10, 1, 13, 8, 9, 4, 5, 11, 12, 7, 2, 14}},
    {{2, 12, 4, 1, 7, 10, 11, 6, 8, 5, 3, 15, 13, 0, 14, 9},
     {14, 11, 2, 12, 4, 7, 13, 1, 5, 0, 15, 10, 3, 9, 8, 6},
     {4, 2, 1, 11, 10, 13, 7, 8, 15, 9, 12, 5, 6, 3, 0, 14},
     {11, 8, 12, 7, 1, 14, 2, 13, 6, 15, 0, 9, 10, 4, 5, 3}},
    {{12, 1, 10, 15, 9, 2, 6, 8, 0, 13, 3, 4, 14, 7, 5, 11},
     {10, 15, 4, 2, 7, 12, 9, 5, 6, 1, 13, 14, 0, 11, 3, 8},
     {9, 14, 15, 5, 2, 8, 12, 3, 7, 0, 4, 10, 1, 13, 11, 6},
     {4, 3, 2, 12, 9, 5, 15, 10, 11, 14, 1, 7, 6, 0, 8, 13}},
    {{4, 11, 2, 14, 15, 0, 8, 13, 3, 12, 9, 7, 5, 10, 6, 1},
     {13, 0, 11, 7, 4, 9, 1, 10, 14, 3, 5, 12, 2, 15, 8, 6},
     {1, 4, 11, 13, 12, 3, 7, 14, 10, 15, 6, 8, 0, 5, 9, 2},
     {6, 11, 13, 8, 1, 4, 10, 7, 9, 5, 0, 15, 14, 2, 3, 12}},
    {{13, 2, 8, 4, 6, 15, 11, 1, 10, 9, 3, 14, 5, 0, 12, 7},
     {1, 15, 13, 8, 10, 3, 7, 4, 12, 5, 6, 11, 0, 14, 9, 2},
     {7, 11, 4, 1, 9, 12, 14, 2, 0, 6, 10, 13, 15, 3, 5, 8},
     {2, 1, 14, 7, 4, 10, 8, 13, 15, 12, 9, 0, 3, 5, 6, 11}},
};

static const uint8_t IP[64] = {58, 50, 42, 34, 26, 18, 10, 2, 60, 52, 44, 36, 28, 20, 12, 4,
                               62, 54, 46, 38, 30, 22, 14, 6, 64, 56, 48, 40, 32, 24, 16, 8,
                               57, 49, 41, 33, 25, 17, 9,  1, 59, 51, 43, 35, 27, 19, 11, 3,
                               61, 53, 45, 37, 29, 21, 13, 5, 63, 55, 47, 39, 31, 23, 15, 7};

static const uint8_t FP[64] = {40, 8, 48, 16, 56, 24, 64, 32, 39, 7, 47, 15, 55, 23, 63, 31,
                               38, 6, 46, 14, 54, 22, 62, 30, 37, 5, 45, 13, 53, 21, 61, 29,
                               36, 4, 44, 12, 52, 20, 60, 28, 35, 3, 43, 11, 51, 19, 59, 27,
                               34, 2, 42, 10, 50, 18, 58, 26, 33, 1, 41, 9,  49, 17, 57, 25};

static const uint8_t P[32] = {16, 7, 20, 21, 29, 12, 28, 17, 1,  15, 23, 26, 5,  18, 31, 10,
                              2,  8, 24, 14, 32, 27, 3,  9,  19, 13, 30, 6,  22, 11, 4,  25};

static const uint8_t PC1[56] = {57, 49, 41, 33, 25, 17, 9,  1,  58, 50, 42, 34, 26, 18,
                                10, 2,  59, 51, 43, 35, 27, 19, 11, 3,  60, 52, 44, 36,
                                63, 55, 47, 39, 31, 23, 15, 7,  62, 54, 46, 38, 30, 22,
                                14, 6,  61, 53, 45, 37, 29, 21, 13, 5,  28, 20, 12, 4};

static const uint8_t PC2[48] = {14, 17, 11, 24, 1,  5,  3,  28, 15, 6,  21, 10, 23, 19, 12, 4,
                                26, 8,  16, 7,  27, 20, 13, 2,  41, 52, 31, 37, 47, 55, 30, 40,
                                51, 45, 33, 48, 44, 49, 39, 56, 34, 53, 46, 42, 50, 36, 29, 32};

static const uint8_t KEY_SHIFTS[DES_ROUNDS] = {1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1};

/* ---- derived lookup tables --------------------------------------------- */

static uint32_t SP[8][64];
static uint64_t IPT[8][256];
static uint64_t FPT[8][256];
static int tables_ready;

/* Generic bit permutation. x holds an in_width-bit value whose DES bit 1 is
 * its MSB; output bit i (1-based, MSB first) is input bit table[i-1]. */
static uint64_t permute(uint64_t x, unsigned in_width, const uint8_t *table, unsigned out_width) {
    uint64_t out = 0;
    for (unsigned i = 0; i < out_width; i++) {
        out |= ((x >> (in_width - table[i])) & 1u) << (out_width - 1 - i);
    }
    return out;
}

void des_init_tables(void) {
    if (tables_ready) {
        return;
    }
    for (unsigned b = 0; b < 8; b++) {
        for (unsigned v = 0; v < 64; v++) {
            unsigned row = ((v >> 4) & 2u) | (v & 1u); /* bits 6 and 1 of the input */
            unsigned col = (v >> 1) & 15u;             /* bits 5..2 */
            uint64_t s = (uint64_t)S[b][row][col] << (28 - 4 * b);
            SP[b][v] = (uint32_t)permute(s, 32, P, 32);
        }
    }
    for (unsigned k = 0; k < 8; k++) {
        for (unsigned x = 0; x < 256; x++) {
            uint64_t v = (uint64_t)x << (56 - 8 * k);
            IPT[k][x] = permute(v, 64, IP, 64);
            FPT[k][x] = permute(v, 64, FP, 64);
        }
    }
    tables_ready = 1;
}

/* ---- key schedule ------------------------------------------------------ */

void des_set_key(des_key_schedule *ks, uint64_t key) {
    const uint32_t half_mask = 0x0FFFFFFFu;
    uint64_t cd = permute(key, 64, PC1, 56);
    uint32_t c = (uint32_t)(cd >> 28) & half_mask;
    uint32_t d = (uint32_t)cd & half_mask;

    des_init_tables();
    for (unsigned r = 0; r < DES_ROUNDS; r++) {
        unsigned s = KEY_SHIFTS[r];
        uint64_t k48;
        c = ((c << s) | (c >> (28 - s))) & half_mask;
        d = ((d << s) | (d >> (28 - s))) & half_mask;
        k48 = permute(((uint64_t)c << 28) | d, 56, PC2, 48);
        for (unsigned b = 0; b < 8; b++) {
            ks->sub[r][b] = (uint8_t)((k48 >> (42 - 6 * b)) & 0x3Fu);
        }
    }
}

/* ---- block encryption -------------------------------------------------- */

static inline uint32_t rotl32(uint32_t x, unsigned n) {
    return (x << n) | (x >> (32 - n));
}

static inline uint64_t initial_permutation(uint64_t x) {
    return IPT[0][x >> 56] ^ IPT[1][(x >> 48) & 0xFF] ^ IPT[2][(x >> 40) & 0xFF] ^
           IPT[3][(x >> 32) & 0xFF] ^ IPT[4][(x >> 24) & 0xFF] ^ IPT[5][(x >> 16) & 0xFF] ^
           IPT[6][(x >> 8) & 0xFF] ^ IPT[7][x & 0xFF];
}

static inline uint64_t final_permutation(uint64_t x) {
    return FPT[0][x >> 56] ^ FPT[1][(x >> 48) & 0xFF] ^ FPT[2][(x >> 40) & 0xFF] ^
           FPT[3][(x >> 32) & 0xFF] ^ FPT[4][(x >> 24) & 0xFF] ^ FPT[5][(x >> 16) & 0xFF] ^
           FPT[6][(x >> 8) & 0xFF] ^ FPT[7][x & 0xFF];
}

/* The Feistel function f(R, K) = P(S(E(R) xor K)), with E done by rotation
 * and S and P folded into the SP tables. */
static inline uint32_t feistel(uint32_t r, const uint8_t *k) {
    return SP[0][(rotl32(r, 5) & 0x3F) ^ k[0]] ^ SP[1][(rotl32(r, 9) & 0x3F) ^ k[1]] ^
           SP[2][(rotl32(r, 13) & 0x3F) ^ k[2]] ^ SP[3][(rotl32(r, 17) & 0x3F) ^ k[3]] ^
           SP[4][(rotl32(r, 21) & 0x3F) ^ k[4]] ^ SP[5][(rotl32(r, 25) & 0x3F) ^ k[5]] ^
           SP[6][(rotl32(r, 29) & 0x3F) ^ k[6]] ^ SP[7][(rotl32(r, 1) & 0x3F) ^ k[7]];
}

static inline uint64_t des_rounds(const des_key_schedule *ks, uint64_t block, int decrypt) {
    uint64_t x = initial_permutation(block);
    uint32_t l = (uint32_t)(x >> 32);
    uint32_t r = (uint32_t)x;

    for (unsigned i = 0; i < DES_ROUNDS; i++) {
        const uint8_t *k = ks->sub[decrypt ? DES_ROUNDS - 1 - i : i];
        uint32_t t = l ^ feistel(r, k);
        l = r;
        r = t;
    }
    /* Pre-output is R16 || L16 (the halves are swapped after the last round). */
    return final_permutation(((uint64_t)r << 32) | l);
}

uint64_t des_encrypt_block(const des_key_schedule *ks, uint64_t block) {
    return des_rounds(ks, block, 0);
}

uint64_t des_decrypt_block(const des_key_schedule *ks, uint64_t block) {
    return des_rounds(ks, block, 1);
}

/* ---- helpers ----------------------------------------------------------- */

int des_is_weak_key(uint64_t key) {
    /* FIPS 74 weak and semi-weak keys, with odd parity. Parity bits are
     * masked off before comparing so any parity variant is recognised. */
    static const uint64_t weak[16] = {
        0x0101010101010101ull, 0xFEFEFEFEFEFEFEFEull, 0xE0E0E0E0F1F1F1F1ull, 0x1F1F1F1F0E0E0E0Eull,
        0x011F011F010E010Eull, 0x1F011F010E010E01ull, 0x01E001E001F101F1ull, 0xE001E001F101F101ull,
        0x01FE01FE01FE01FEull, 0xFE01FE01FE01FE01ull, 0x1FE01FE00EF10EF1ull, 0xE01FE01FF10EF10Eull,
        0x1FFE1FFE0EFE0EFEull, 0xFE1FFE1FFE0EFE0Eull, 0xE0FEE0FEF1FEF1FEull, 0xFEE0FEE0FEF1FEF1ull,
    };
    const uint64_t parity_mask = 0xFEFEFEFEFEFEFEFEull;
    for (unsigned i = 0; i < 16; i++) {
        if ((key & parity_mask) == (weak[i] & parity_mask)) {
            return 1;
        }
    }
    return 0;
}

uint64_t des_load_be64(const uint8_t *p) {
    uint64_t v = 0;
    for (unsigned i = 0; i < 8; i++) {
        v = (v << 8) | p[i];
    }
    return v;
}

void des_store_be64(uint8_t *p, uint64_t v) {
    for (unsigned i = 0; i < 8; i++) {
        p[i] = (uint8_t)(v >> (56 - 8 * i));
    }
}
