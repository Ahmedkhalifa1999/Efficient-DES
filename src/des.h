/* des.h - DES block cipher (FIPS 46-3), table-driven implementation.
 *
 * Conventions: a 64-bit block or key is held in a uint64_t with DES bit 1 as
 * the most significant bit, i.e. the first byte of the block is the top byte
 * of the word. Use des_load_be64/des_store_be64 to convert to and from bytes.
 *
 * DES is an obsolete cipher (56-bit key, withdrawn by NIST in 2005). This
 * implementation exists for educational purposes only.
 */
#ifndef DES_H
#define DES_H

#include <stddef.h>
#include <stdint.h>

#define DES_BLOCK_BYTES 8
#define DES_ROUNDS 16

typedef struct {
    /* Round subkeys, each split into the eight 6-bit S-box inputs. */
    uint8_t sub[DES_ROUNDS][8];
} des_key_schedule;

/* Build the lookup tables. Idempotent. Called by des_set_key; call it
 * explicitly first if several threads may call des_set_key concurrently. */
void des_init_tables(void);

/* Expand a 64-bit key (parity bits ignored) into the round subkeys. */
void des_set_key(des_key_schedule *ks, uint64_t key);

uint64_t des_encrypt_block(const des_key_schedule *ks, uint64_t block);
uint64_t des_decrypt_block(const des_key_schedule *ks, uint64_t block);

/* Non-zero if the key is one of the 4 weak or 12 semi-weak DES keys. */
int des_is_weak_key(uint64_t key);

uint64_t des_load_be64(const uint8_t *p);
void des_store_be64(uint8_t *p, uint64_t v);

#endif /* DES_H */
