# Code review: Efficient-DES (`des_gp1.cpp`)

Reviewed at commit `e3c9632` ("Final Submission"), 2026-09-18. The remediation plan is in [FIX_PLAN.md](FIX_PLAN.md); it refers to the findings below as F1 to F12.

## Verdict

The DES core is correct: all four classic known-answer vectors pass, and the ciphertext of 64 MiB of random data is byte-identical to OpenSSL's DES-ECB. Everything around the core is fragile. The program silently drops trailing bytes of any input that is not a multiple of 8 bytes, segfaults on any missing file in the default build, silently encrypts with an all-zero key when the key file is malformed, and exits 0 on every error path. The "efficient" claim does not hold up: the per-core throughput is about 16x below a standard table-driven DES, and the multithreading only hides that on multi-core machines. The repository also lacks a README, a build file, tests, and a license, and it ships a compiled binary in git.

## What works (verified)

| Check | Result |
|---|---|
| FIPS 46 vector, key `133457799BBCDFF1`, pt `0123456789ABCDEF` | `85E813540F0AB405` (correct) |
| All-zero key and plaintext | `8CA64DE9C1B123A7` (correct) |
| key `0123456789ABCDEF`, pt `"Now is t"` | `3FA40E8A984D4815` (correct) |
| All-ones key and plaintext | `7359B2163E4EDC58` (correct) |
| 1 MiB, 786 KiB, 64 MiB random files vs `openssl enc -des-ecb -nopad` | byte-identical |
| decrypt(encrypt(x)) == x on all of the above | yes |
| AddressSanitizer + UBSan on valid inputs | clean |
| ThreadSanitizer (encrypt and decrypt, multi-chunk input) | clean, no races |
| Key file with trailing newline, lowercase hex | accepted |
| Committed `des` binary vs fresh build | same output, same crashes |

Design points that deserve credit: the whole block lives in one `unsigned long long`, the Feistel round is a single expression (`des_gp1.cpp:297`), the key schedule uses masked rotates instead of bit arrays (`des_gp1.cpp:300-316`), each thread owns a disjoint row of the buffer so there is no shared-write race, and threading scales almost linearly (3.7x on 4 cores).

## Findings, most severe first

### 1. Silent data loss on inputs that are not a multiple of 8 bytes (`des_gp1.cpp:429`, `:496`)

`fread(data, sizeof(unsigned long long), N, file)` returns only whole 8-byte items. Trailing bytes are dropped, no padding is applied, and the program prints "Encryption Done" with exit code 0.

```
$ des encrypt odd13.bin key1.txt out.ct   # 13-byte input
Encryption Done
$ stat -c%s out.ct
8
```

A text file, an image, or any real document will almost never be a multiple of 8 bytes, so this affects nearly every real input. Fix: apply PKCS#7 padding on encrypt and strip it on decrypt, or at minimum detect `ftell`/`fread` remainder and abort with a message.

### 2. Segfault on any missing or unopenable file in the default build (`des_gp1.cpp:10`, `:400-428`, `:470-495`)

`ERROR_CHECKS` defaults to 0, so `fopen` results are never checked and `fscanf(NULL, ...)` is called. UBSan reports "null pointer passed as argument 1". Observed with a missing key file, a missing input file, and an unwritable output path: exit code 139, no message. Even with `ERROR_CHECKS 1` the error paths `return 0`, so a script cannot detect failure. Error checks on three `fopen` calls cost nothing; they should be unconditional and return non-zero.

### 3. Malformed keys are silently accepted as key zero (`des_gp1.cpp:409`, `:479`)

`fscanf` return value is ignored. An empty key file, a file containing `hello`, or a directory passed as the key file all "succeed" and encrypt with key `0000000000000000` (output `617B3A0CE8F07100` for the FIPS plaintext in every case). An 8-character key `12345678` is accepted as `0000000012345678`. Encrypt uses `%016llX`, decrypt uses `%llX`; the formats differ for no reason. Use `fgets` + `strtoull` with `endptr` and length checks, and reject anything that is not exactly 16 hex digits.

### 4. Exit code is 0 on every path (`des_gp1.cpp:526`, `:531`)

"Invalid Arguments" returns 0. No usage text is printed, so the only documentation of the CLI (`des encrypt <in> <keyfile> <out>`) is the source of `main`.

### 5. `OUTPUT_HEX` debug path prints stale and byte-reversed data (`des_gp1.cpp:370-376`, `:451-456`)

`fputs(cipherHex[i], ...)` runs for all 16 threads on every chunk, but only the threads that had blocks in the current chunk rewrote their buffers. For a 256 KiB + 8 byte input the expected 32769 hex lines came out as 63489: the last chunk used one thread, and the other 15 threads' buffers from the previous chunk were printed again. The values are also printed after the second byte swap, so each line is the byte-reversed form of the ciphertext actually written to the file (`359A42674D9DD745` in the hex dump vs `45D79D4D67429A35` in the output file). The output filename `cipherHEX.txt` is hardcoded.

### 6. `OUTPUT_TIME_TAKEN` reports CPU time, not elapsed time (`des_gp1.cpp:396`, `:528-529`)

`clock()` sums CPU time across all threads. On a 1 MiB input it reported 0.211 s while wall time was 0.068 s. For a project whose stated goal is performance, the built-in timer overstates runtime by the thread count. Use `clock_gettime(CLOCK_MONOTONIC, ...)`.

### 7. The "efficient" claim: about 16x slower per core than a table-driven DES

64 MiB of random data, `-O3`, 4 CPUs:

| Configuration | Wall | User CPU | Throughput (wall) | Throughput per CPU-second |
|---|---|---|---|---|
| this code, `THREAD_COUNT=1` | 14.91 s | 14.14 s | 4.3 MB/s | 4.5 MB/s |
| this code, `THREAD_COUNT=4` | 3.86 s | 12.66 s | 16.6 MB/s | 5.1 MB/s |
| this code, `THREAD_COUNT=16` (as shipped) | 4.00 s | 12.15 s | 16.0 MB/s | 5.3 MB/s |
| OpenSSL 3.0 `des-ecb`, single thread | 0.91 s | 0.78 s | 70.5 MB/s | 82.5 MB/s |

Causes, all in the per-block path:

- `InitialPermutation` and `InverseInitialPermutation` loop 64 times per block with a shift, mask, shift and or per bit (`des_gp1.cpp:134-156`).
- `ExpansionPermutation` loops 48 times per round (`:158-169`), and `Permutation` 32 times per round (`:281-293`). That is 16 x 80 = 1280 iterations per block, plus the 128 for IP and FP: about 1400 dependent bit operations per 8 bytes.
- The S-box step extracts eight 6-bit fields with 48-bit masks and eight near-identical functions (`:172-237`, `:264-276`).

The standard approach (used by OpenSSL, libdes, and every textbook fast DES) precomputes eight 64-entry 32-bit tables that combine each S-box with the P permutation, so a round is eight table lookups and XORs. E becomes a handful of shifts and masks on the 32-bit half, and IP/FP become either byte-indexed tables or the well-known five-step bit-swap sequence. This typically gives a 10-20x per-core improvement and is what "efficient DES" normally means. The current code gets its speed only from running the slow kernel on many cores.

### 8. Thread usage is ad hoc (`des_gp1.cpp:21`, `:429-459`)

- `THREAD_COUNT` is hardcoded to 16. On this 4-CPU host the 4-thread build is as fast as the 16-thread one; on a 2-core laptop 16 threads just add contention.
- Sixteen threads are created and joined for every 256 KiB chunk (4096 thread creations for 64 MiB). A pool sized to `sysconf(_SC_NPROCESSORS_ONLN)` with larger work items would remove that overhead.
- Reading, computing, and writing are strictly sequential per chunk. No I/O overlaps computation.
- `pthread_create` return values are ignored (`:440`, `:507`); `fwrite` return values are ignored (`:450`, `:518`).
- `ThreadNumber` is a `char` used as an array index (12 `-Wchar-subscripts` warnings).

### 9. Inconsistent bit-numbering convention, correct by coincidence (`des_gp1.cpp:134-156` vs `:111-132`, `:158-169`, `:281-293`)

PC-1, PC-2, E and P index bit 1 as the most significant bit (`key >> (64 - keyPC1[i])`). IP and IP^-1 index bit 1 as the least significant bit (`1ULL << (table[i]-1)`). The two conventions produce the same result only because the DES IP table satisfies `IP[65-i] = 65 - IP[i]`, so it commutes with 64-bit reversal. Any other table pasted into that loop would give wrong output with no warning. The byte-swap in `IHateEndinanness` (`:342-348`) is a separate, legitimate step (file bytes are big-endian in DES), but it is a hand-written `bswap64`; use `__builtin_bswap64` or `be64toh`/`htobe64`.

### 10. Code quality

- 23 warnings under `-Wall -Wextra` on both compilers. Six are `-Wparentheses` on expressions like `x << 55-i` and `y >> 16 + i` (`:117`, `:128`, `:166`, `:290`, `:306`, `:309`). They evaluate as intended because `-`/`+` bind tighter than shifts, but both compilers flag them as a hazard.
- Four unused constants in `SBox` (`:242-245`).
- Eight copy-pasted `Sn_API` functions that differ only in the table (`:172-237`); one function taking a table pointer, or a single `S[8][4][16]` array, replaces them.
- The file is C (`stdio.h`, `pthread.h`, no C++ constructs; compiles with `gcc -x c -std=gnu11`) but named `.cpp`. Pick one language.
- Typos in identifiers: `MUSK`, `coloumn`, `expended`, `IHateEndinanness`. `FeistelFunction` is actually a full Feistel round, not the F function.
- Mixed tabs and spaces, mixed brace styles, magic numbers throughout (clang-tidy: 68 `readability-magic-numbers`, 29 `bugprone-narrowing-conversions`, 10 `cert-err33-c` unchecked returns).
- All functions have external linkage; `keys` and `data` are mutable globals, so the code is not reusable as a library.
- `.gitignore` ignores a file named `test` and nothing else.

### 11. Repository hygiene

- A compiled ELF (`des`, built with GCC 9.4 on Ubuntu 20.04) is committed. Earlier history committed `test.exe` in six revisions. Binaries do not belong in git; add them to `.gitignore`.
- No README. The only usage notes are a comment at the top of the source, and they name the output `des_gp1` while the committed binary is `des`.
- No Makefile or CMake file, no test script, no CI, no license.
- Commit messages such as "Done", "Dones", "out", "It works", "breksoo top" carry no information.

### 12. Cryptographic context (not a bug in the assignment, but should be stated in a README)

DES has a 56-bit key and has been withdrawn by NIST since 2005; ECB mode leaks block-level patterns and there is no integrity check. This is fine for an educational implementation, but the README should say so, and the program should not be presented as a file-encryption tool.

## Recommended order of fixes

1. Padding for partial blocks; unconditional `fopen` checks; non-zero exit codes; usage message.
2. Strict key parsing with `strtoull` and length validation; one format for encrypt and decrypt.
3. Fix or delete the `OUTPUT_HEX` path; switch timing to a monotonic wall clock.
4. Replace the bit-loop permutations with combined S/P tables and shift-based E; then re-benchmark single-threaded against `openssl speed des-ecb`.
5. Thread pool sized to the machine, larger work units, double-buffered I/O.
6. Remove the binary, add README, Makefile, a test script that runs the four KATs and an OpenSSL cross-check, and compile with `-Wall -Wextra -Werror`.

## Appendix: reproduction

```
# build
g++ -Wall -Wextra -pthread -O3 -o des_gp1 des_gp1.cpp

# known-answer test
printf '133457799BBCDFF1' > key.txt
python3 -c "import sys; sys.stdout.buffer.write(bytes.fromhex('0123456789ABCDEF'))" > pt.bin
./des_gp1 encrypt pt.bin key.txt ct.bin && od -An -tx1 ct.bin      # 85 e8 13 54 0f 0a b4 05

# OpenSSL cross-check (OpenSSL 3 needs the legacy provider for single DES)
head -c 1048576 /dev/urandom > r.bin
./des_gp1 encrypt r.bin key.txt r.ct
openssl enc -provider legacy -provider default -des-ecb -K 133457799BBCDFF1 -nopad -in r.bin -out r.ossl
cmp r.ct r.ossl && echo MATCH

# partial-block loss
head -c 13 /dev/urandom > odd.bin; ./des_gp1 encrypt odd.bin key.txt odd.ct; stat -c%s odd.ct   # 8

# crash on missing key file
./des_gp1 encrypt pt.bin /nonexistent key.ct; echo $?   # 139

# sanitizers
g++ -g -O1 -fsanitize=address,undefined -pthread -o des_asan des_gp1.cpp
g++ -g -O1 -fsanitize=thread -pthread -o des_tsan des_gp1.cpp
```
