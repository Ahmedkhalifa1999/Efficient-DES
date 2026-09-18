# Fix plan for Efficient-DES

Companion to [REVIEW.md](REVIEW.md). Findings are referenced as F1 to F12 (the numbered
sections of the review). Phases are ordered so that each phase is protected by the tests
added in the phases before it. Effort figures are rough estimates for one person who knows C.

## Guiding decisions

- **Tests before fixes.** Phase 0 adds a build file, a test script, and CI. Every later phase
  must keep `make test` green.
- **Keep the CLI compatible where it is cheap.** The positional syntax
  `des encrypt|decrypt <in> <keyfile> <out>` stays. New behaviour goes behind flags.
- **Padding changes the ciphertext format.** After Phase 1 the tool pads with PKCS#7, so an
  8-byte-aligned input gains one extra block. A `--nopad` flag keeps today's raw behaviour
  for old ciphertexts and for the OpenSSL `-nopad` cross-check.
- **Generate tables from the verified code.** Phase 3 derives every lookup table at start-up
  from the existing, verified bit-loop functions, so no new bit-order reasoning is needed and
  the KATs catch any mistake.
- **Language.** The file uses no C++ (see F10). Recommendation: rename to `des.c` and build as
  C11. If C++ is preferred, use `<cstdint>`, `std::thread` and `std::chrono` instead.

## Phase 0: Safety net (F10 warnings, F11 hygiene)

Goal: every later change is verified automatically.

| # | Task | Where |
|---|---|---|
| 0.1 | Add `Makefile` with targets `all` (`gcc -std=gnu11 -O3 -pthread -Wall -Wextra -Werror`), `asan` (`-O1 -g -fsanitize=address,undefined`), `tsan` (`-fsanitize=thread`), `test`, `bench`, `clean`. | new `Makefile` |
| 0.2 | Add `tests/run_tests.sh`: builds, runs the four KATs from REVIEW.md, round-trips random files of 8-aligned sizes (0, 8, 4096, 262144, 262152, 786456, 1048576 bytes), and cross-checks against `openssl enc -provider legacy -provider default -des-ecb -nopad` when that works on the host. Exit non-zero on any failure. | new `tests/` |
| 0.3 | Delete the committed `des` binary. Extend `.gitignore` with `des`, `des_gp1`, `build/`, `*.o`, `cipherHEX.txt`, `tests/tmp/`. History rewriting is optional and not recommended for a shared repo. | `des`, `.gitignore` |
| 0.4 | Add a GitHub Actions workflow: on push and PR, `make all`, `make test`, `make asan && make test`. | new `.github/workflows/ci.yml` |
| 0.5 | Make the build warning-clean so `-Werror` passes: parenthesise the shift operands at `des_gp1.cpp:117`, `:128`, `:166`, `:290`, `:306`, `:309`; change `ThreadNumber` and the `char` loop counters (`:160`, `:356`) to `int`; delete the unused constants at `:242-245`; check the `fscanf` return until Phase 1 replaces it. | `des_gp1.cpp` |

Acceptance: `make test` passes locally and in CI; `git ls-files` lists no binaries; gcc and
clang both build with `-Werror`.
Effort: about 3 hours.

## Phase 1: Correctness and robustness (F1 to F4)

| # | Task | Detail |
|---|---|---|
| 1.1 | Padding (F1) | PKCS#7. Encrypt: at EOF append `p = 8 - (len % 8)` bytes of value `p` (1 to 8), so a full block of `0x08` is added when the input is already aligned. Decrypt: the pad is in the last block, so defer writing the final block of each chunk until the next `fread` shows whether more data follows; then require `1 <= p <= 8` and all `p` trailing bytes equal to `p`, else exit 2 with "bad padding (wrong key or corrupt input)". Switch the I/O buffer to bytes so `len % 8` is known. Add `--nopad`: raw mode that keeps today's behaviour but rejects inputs that are not a multiple of 8 instead of truncating. |
| 1.2 | File and I/O errors (F2) | Delete the `ERROR_CHECKS` macro. Always check `fopen`, `fread` (`ferror`), `fwrite` (short write), `fclose`, and `pthread_create`. Print `des: <path>: <strerror(errno)>` to stderr and exit 1. |
| 1.3 | Key parsing (F3) | Read the key file with `fgets` into a 64-byte buffer, trim trailing whitespace, require exactly 16 hex digits (`strspn(s, "0123456789abcdefABCDEF") == 16 && s[16] == '\0'`), parse with `strtoull(s, &end, 16)`. Anything else exits 2 with "key file must contain exactly 16 hex digits". One code path for encrypt and decrypt. Optional: warn on the 4 weak and 12 semi-weak keys. |
| 1.4 | Exit codes and usage (F4) | `usage()` to stderr and exit 2 on bad arguments; exit 0 only on success. Add `-h`/`--help`, `--nopad`, `--threads N`, `--time`. |

Tests to add: round-trip of sizes 1, 7, 9, 13, 4097 and 262153; decrypt with the wrong key
exits non-zero; missing key, input or output path exits non-zero with a message; key files
that are empty, `hello`, 8 characters, 17 characters, lowercase, with trailing newline, and with
CRLF; padded output matches `openssl enc -des-ecb` without `-nopad` (OpenSSL also uses PKCS#7,
so the files must be byte-identical); `--nopad` output matches `openssl ... -nopad`.

Acceptance: all of the above pass, and this works for a 13-byte file:

```
des encrypt f.txt k.txt f.ct && des decrypt f.ct k.txt g.txt && cmp f.txt g.txt
```

Effort: 4 to 6 hours.

## Phase 2: Diagnostics (F5, F6)

| # | Task | Detail |
|---|---|---|
| 2.1 | Hex output (F5) | Replace the `OUTPUT_HEX` macro with a `--hex <file>` option, or delete it. If kept: format from the main thread after the join, over exactly `maximum` blocks of the current chunk, printing the 8 bytes in file order with `%02X` rather than the host-order 64-bit word. |
| 2.2 | Timing (F6) | Replace `clock()` with `clock_gettime(CLOCK_MONOTONIC, ...)` behind `--time`; report wall time, and CPU time separately from `getrusage` if wanted. |

Acceptance: for a 256 KiB + 8 byte input the hex file has exactly 32769 lines and its last line
equals `od -An -tx1` of the last 8 bytes of the output; `--time` agrees with `time` within 5 %.
Effort: 1 to 2 hours.

## Phase 3: Cipher core performance (F7, F9)

Target: single-thread throughput within 2x of `openssl speed des-ecb` on the same machine
(on the review host that means at least 35 MB/s against today's 4.3 MB/s).

Run `make test` after every step.

| # | Task | Detail |
|---|---|---|
| 3.1 | Combined S-box and P tables | `static uint32_t SP[8][64]`, filled in an init function from the existing code: `SP[b][v] = Permutation((uint64_t)S_b(v) << (28 - 4*b))` for `b` in 0..7 and `v` in 0..63, where `S_b` is `S1_API`..`S8_API`. This is valid because P is a bit permutation and the eight inputs are disjoint. |
| 3.2 | Expansion by rotation | With `R` the 32-bit right half, the 6-bit input of box `b` is `ROTL32(R, 5 + 4*b) & 0x3F` for `b` in 0..7 (for `b = 7` that is `ROTL32(R, 1)`). Add a unit test that checks this against `ExpansionPermutation` for 100000 random `R` before relying on it. |
| 3.3 | Pre-split round keys | `uint8_t ks[16][8]`, `ks[i][b] = (keys[i] >> (42 - 6*b)) & 0x3F`. |
| 3.4 | New round | `f = SP[0][(ROTL32(R,5) & 63) ^ ks[i][0]] ^ SP[1][(ROTL32(R,9) & 63) ^ ks[i][1]] ^ ... ^ SP[7][(ROTL32(R,1) & 63) ^ ks[i][7]]; newL = R; newR = L ^ f;`. Keep `L` and `R` in two `uint32_t` for all 16 rounds instead of packing and unpacking a 64-bit word each round. |
| 3.5 | IP and FP via byte tables | `uint64_t IPT[8][256]`, `IPT[k][byte] = InitialPermutation((uint64_t)byte << (56 - 8*k))`; then `IP(x) = IPT[0][x >> 56] ^ IPT[1][(x >> 48) & 255] ^ ... ^ IPT[7][x & 255]`. Same for FP. 16 KiB per table set, cache resident. Optional refinement later: the five delta-swap steps used by OpenSSL and Outerbridge (masks `0x0F0F0F0F`/4, `0x0000FFFF`/16, `0x33333333`/2, `0x00FF00FF`/8, `0x55555555`/1); the operand order depends on word orientation, so derive it and confirm with the KATs. |
| 3.6 | Remove dead code | Delete the bit-loop `ExpansionPermutation`, `Permutation`, `SBox` and the eight `Sn_API` copies from the hot path; keep one `S[8][4][16]` table as the source of truth for the generator. |
| 3.7 | Bit-numbering convention (F9) | After 3.5 only the table generators use bit loops. Document one convention (bit 1 = MSB) and rewrite the IP/FP generator loops in that convention so they no longer depend on the IP table being symmetric under reversal. |
| 3.8 | Byte swap | Replace `IHateEndinanness` with `__builtin_bswap64` (or `be64toh`/`htobe64` from `<endian.h>`). |

Acceptance: KATs and OpenSSL cross-check pass; `make bench` on 64 MiB shows at least 8x
single-thread improvement.
Effort: about one day.

## Phase 4: Threading and I/O (F8)

| # | Task | Detail |
|---|---|---|
| 4.1 | Thread count | `--threads N`, default `sysconf(_SC_NPROCESSORS_ONLN)`, capped at the number of blocks. |
| 4.2 | Work distribution | Regular-file fast path first: `mmap` the input (or read it fully when it is small), partition the blocks into N contiguous ranges, one thread per range writing into an output buffer, one `fwrite` per range. This removes the per-256-KiB spawn/join loop entirely. Only if stdin or pipes are wanted: a pool of N long-lived threads over a queue of 4 to 16 MiB chunks, double-buffered so reading chunk k+1 overlaps computing chunk k and writing chunk k-1. |
| 4.3 | No mutable globals after start-up | Put `ks`, the tables and the buffers in a context struct passed to the threads, or make them `static` and written only before threads start. |

Acceptance: TSan clean; wall time on 64 MiB is within 20 % of single-thread time divided by
`min(N, cores)`; 1-thread and N-thread outputs identical.
Effort: about half a day.

## Phase 5: Code quality (F10)

| # | Task |
|---|---|
| 5.1 | Rename to `des.c`, build as C11 (or commit to C++ properly, see Guiding decisions). |
| 5.2 | Use `<stdint.h>` types (`uint64_t`, `uint32_t`, `uint8_t`) instead of `unsigned long long`, `long` and `char`. |
| 5.3 | Rename: `MUSK` (delete, unused after Phase 3), `coloumn` to `column`, `expended` to `expanded`, `FeistelFunction` to `feistel_round`, `SBox` to `sbox_substitute`; pick one naming style. |
| 5.4 | `static` on every internal function; `const` where clang-tidy suggests; named constants for `DES_BLOCK_BYTES`, `DES_ROUNDS`, `HALF_MASK` and the key-schedule masks. |
| 5.5 | Add `.clang-format` (4-space indent) and format once; run clang-tidy with `bugprone-*,cert-*,performance-*,misc-*` and fix what remains. |
| 5.6 | Split into `des.c`/`des.h` (key schedule, block encrypt/decrypt, no I/O) and `main.c` (CLI, files, threads), and add `tests/kat.c` linking `des.c` so the cipher is unit-tested without the CLI. |

Acceptance: `-Wall -Wextra -Werror -Wconversion` clean on gcc and clang; no bugprone or cert
findings from clang-tidy.
Effort: about half a day.

## Phase 6: Documentation (F11, F12)

| # | Task |
|---|---|
| 6.1 | `README.md`: purpose (educational DES, course project), build, exact usage, key-file format, padding and `--nopad`, threads, before/after benchmark table, and a security notice: DES has a 56-bit key and was withdrawn by NIST in 2005, ECB leaks block patterns, there is no integrity check, do not use for real data. |
| 6.2 | `LICENSE` (MIT, or whatever the course requires). |
| 6.3 | Replace the stale header comment in the source (it names the binary `des_gp1`) with a pointer to the README. |

Effort: 1 to 2 hours.

## Definition of done

- [ ] No binaries tracked; `.gitignore` covers build outputs
- [ ] `make`, `make test`, CI green with `-Werror`; ASan/UBSan and TSan builds pass the tests
- [ ] Any input size round-trips; `--nopad` rejects sizes that are not a multiple of 8
- [ ] Every error path prints a message and exits non-zero; invalid key files are rejected
- [ ] Hex and timing diagnostics are correct or removed
- [ ] Single-thread throughput at least 8x today's; multi-thread scales with cores
- [ ] Source split into cipher and CLI, C11 clean, formatted, unit-tested
- [ ] README with usage and security notice; LICENSE present

## Suggested pull-request sequence

One PR per phase, in order. Phase 0 first, because it protects everything after it.
Phases 1 and 2 can be reviewed together. Phase 3 is the largest change and the one that
benefits most from the test suite being in place first.
