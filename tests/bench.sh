#!/usr/bin/env bash
# Throughput benchmark: 64 MiB of random data, 1 thread and all CPUs, plus
# OpenSSL single-thread DES-ECB for reference when available.
#   usage: tests/bench.sh [path-to-des-binary] [MiB]
set -u
DES=$(realpath "${1:-./des}")
MIB=${2:-64}
TMP=$(mktemp -d "${TMPDIR:-/tmp}/des-bench.XXXXXX")
trap 'rm -rf "$TMP"' EXIT
cd "$TMP"
head -c $((MIB * 1024 * 1024)) /dev/urandom > in.bin
printf '133457799BBCDFF1' > key.txt
TIMEFORMAT='%R'
run() { # label command...
    local label=$1; shift
    "$@" >/dev/null 2>&1                       # warm-up
    local t; t=$( { time "$@" >/dev/null 2>&1; } 2>&1 )
    printf '%-36s %7.3f s  %8.1f MB/s\n' "$label" "$t" "$(echo "$MIB * 1048576 / $t / 1000000" | bc -l)"
}
echo "input: $MIB MiB, cpus: $(nproc)"
run "des encrypt --threads 1"            "$DES" encrypt in.bin key.txt out.bin --threads 1
run "des encrypt --threads $(nproc)"     "$DES" encrypt in.bin key.txt out.bin
run "des decrypt --threads $(nproc)"     "$DES" decrypt out.bin key.txt back.bin
cmp -s in.bin back.bin || echo "ERROR: round trip mismatch"
if openssl enc -provider legacy -provider default -des-ecb -K 0 -in /dev/null >/dev/null 2>&1; then
    run "openssl enc -des-ecb (1 thread)" openssl enc -provider legacy -provider default -des-ecb -K 133457799BBCDFF1 -in in.bin -out ossl.bin
    cmp -s out.bin ossl.bin && echo "openssl output: identical" || echo "ERROR: openssl output differs"
fi
