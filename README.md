# Efficient-DES

A small, fast DES implementation in C11 with a command-line tool that encrypts
and decrypts files in ECB mode, using all available CPU cores.

> **Security notice.** DES has a 56-bit key and was withdrawn by NIST in 2005;
> it can be brute-forced with modest hardware. ECB mode leaks block-level
> patterns, and this tool adds no integrity protection. It exists as a course
> project and for education. Do not use it to protect real data.

## Build

Requires a C11 compiler and POSIX threads.

```
make            # builds ./des with -O3 and warnings as errors
make test       # unit tests (tests/kat.c) and end-to-end tests (tests/run_tests.sh)
make test-asan  # the same under AddressSanitizer and UBSan
make test-tsan  # the same under ThreadSanitizer
make bench      # 64 MiB throughput, 1 thread vs all CPUs vs OpenSSL
```

## Usage

```
des encrypt <input> <keyfile> <output> [options]
des decrypt <input> <keyfile> <output> [options]

  --nopad        raw mode: no padding; input must be a multiple of 8 bytes
  --threads N    worker threads (default: number of online CPUs)
  --time         print elapsed wall time and throughput to stderr
  --hex FILE     also write every processed block as a 16-digit hex line to FILE
  -h, --help     show help
```

The key file is a text file holding the 64-bit key as 16 hexadecimal digits,
for example `133457799BBCDFF1`. Case and surrounding whitespace do not matter.
Anything else is rejected. The 16 weak and semi-weak keys produce a warning.

Exit status is 0 on success, 1 for an I/O error, and 2 for a usage, key or
padding error. Errors are reported on stderr.

```
$ printf '133457799BBCDFF1' > key.txt
$ des encrypt report.pdf key.txt report.des
$ des decrypt report.des key.txt report2.pdf && cmp report.pdf report2.pdf
```

### Padding and compatibility

Inputs are padded with PKCS#7, so any file size round-trips and the output is
byte-identical to `openssl enc -des-ecb -K <key>`. Decryption verifies the
padding, so a wrong key is reported instead of producing garbage.

`--nopad` keeps the raw behaviour of the original tool (no padding, output the
same size as the input), which matches `openssl enc -des-ecb -nopad`. Use it to
decrypt files produced before padding was added. In raw mode an input whose
size is not a multiple of 8 bytes is an error rather than being truncated.

## Performance

64 MiB of random data, 4 CPUs, `-O3`:

| Implementation | Threads | Throughput |
|---|---|---|
| original `des_gp1.cpp` (bit-loop permutations) | 1 | 4.3 MB/s |
| original `des_gp1.cpp` | 16 | 16.0 MB/s |
| this version | 1 | 91.9 MB/s |
| this version | 4 | 236 MB/s |
| `openssl enc -des-ecb` (OpenSSL 3.0) | 1 | 75.7 MB/s |

The speed comes from the usual table-driven construction: the P permutation is
folded into eight 64-entry S-box tables, the expansion E is done with
rotations, and IP and IP^-1 are eight byte-indexed lookups each. All tables
are generated at start-up from the textbook DES tables in `src/des.c`.

## Layout

```
src/des.h, src/des.c   the cipher: key schedule, block encrypt/decrypt, no I/O
src/main.c             the command-line tool: files, padding, threads
tests/kat.c            unit tests against known-answer vectors and a reference DES
tests/run_tests.sh     end-to-end tests of the tool, including an OpenSSL cross-check
tests/bench.sh         throughput benchmark
REVIEW.md, FIX_PLAN.md the code review of the original version and the plan this follows
```

## License

MIT, see [LICENSE](LICENSE).
