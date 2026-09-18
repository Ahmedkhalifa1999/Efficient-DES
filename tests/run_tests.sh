#!/usr/bin/env bash
# End-to-end tests for the des CLI.
#   usage: tests/run_tests.sh [path-to-des-binary]
# Exit status is non-zero if any check fails.
set -u

DES=${1:-./des}
DES=$(realpath "$DES")
TMP=$(mktemp -d "${TMPDIR:-/tmp}/des-tests.XXXXXX")
trap 'rm -rf "$TMP"' EXIT
cd "$TMP"

pass=0
fail=0
ok()   { pass=$((pass + 1)); }
bad()  { fail=$((fail + 1)); echo "FAIL: $*"; }
check() { # check <description> <command...>   (command must succeed)
    local d=$1; shift
    if "$@" >/dev/null 2>&1; then ok; else bad "$d"; fi
}
hexof() { od -An -tx1 "$1" | tr -d ' \n' | tr a-f A-F; }
mkbin() { printf "$1" > "$2"; }   # $1 has \x escapes

# ---- known-answer tests (raw mode, one block) ------------------------------
kat() { # key plain-hex-bytes expected
    printf '%s' "$1" > k.txt
    mkbin "$2" p.bin
    "$DES" encrypt p.bin k.txt c.bin --nopad 2>/dev/null   # some KAT keys are weak keys
    [ "$(hexof c.bin)" = "$3" ] || bad "KAT key=$1: got $(hexof c.bin) want $3"
    "$DES" decrypt c.bin k.txt d.bin --nopad 2>/dev/null
    cmp -s p.bin d.bin || bad "KAT key=$1: decrypt does not round-trip"
    ok
}
kat 133457799BBCDFF1 '\x01\x23\x45\x67\x89\xab\xcd\xef' 85E813540F0AB405
kat 0000000000000000 '\x00\x00\x00\x00\x00\x00\x00\x00' 8CA64DE9C1B123A7
kat 0123456789ABCDEF 'Now is t'                         3FA40E8A984D4815
kat FFFFFFFFFFFFFFFF '\xff\xff\xff\xff\xff\xff\xff\xff' 7359B2163E4EDC58

printf '133457799BBCDFF1' > key.txt

# ---- padding and round trip for every size class --------------------------
for size in 0 1 7 8 9 13 15 16 17 4095 4096 4097 65536 262144 262152 786456 1048576; do
    head -c "$size" /dev/urandom > in.bin
    "$DES" encrypt in.bin key.txt ct.bin || bad "encrypt size=$size failed"
    want=$(( (size / 8 + 1) * 8 ))
    [ "$(stat -c%s ct.bin)" = "$want" ] || bad "size=$size: ciphertext is $(stat -c%s ct.bin) bytes, want $want"
    "$DES" decrypt ct.bin key.txt out.bin || bad "decrypt size=$size failed"
    cmp -s in.bin out.bin && ok || bad "size=$size: round trip mismatch"
done

# ---- raw mode ---------------------------------------------------------------
head -c 4096 /dev/urandom > al.bin
"$DES" encrypt al.bin key.txt al.ct --nopad && [ "$(stat -c%s al.ct)" = 4096 ] && ok || bad "--nopad keeps size"
"$DES" decrypt al.ct key.txt al.out --nopad && cmp -s al.bin al.out && ok || bad "--nopad round trip"
head -c 13 /dev/urandom > odd.bin
"$DES" encrypt odd.bin key.txt odd.ct --nopad 2>err.txt; [ $? -eq 2 ] && grep -q "multiple of 8" err.txt && ok || bad "--nopad must reject a 13-byte input"
"$DES" decrypt odd.bin key.txt odd.out 2>err.txt;         [ $? -eq 2 ] && ok || bad "decrypt must reject a 13-byte ciphertext"
: > empty.bin
"$DES" encrypt empty.bin key.txt e.ct && [ "$(stat -c%s e.ct)" = 8 ] && ok || bad "empty input gives one pad block"
"$DES" decrypt e.ct key.txt e.out && [ "$(stat -c%s e.out)" = 0 ] && ok || bad "pad-only ciphertext decrypts to empty"
"$DES" encrypt empty.bin key.txt e2.ct --nopad && [ "$(stat -c%s e2.ct)" = 0 ] && ok || bad "--nopad empty input"
"$DES" decrypt empty.bin key.txt e3.out 2>/dev/null; [ $? -eq 2 ] && ok || bad "padded decrypt of empty file must fail"

# ---- OpenSSL cross-check (skipped if this OpenSSL cannot do single DES) -----
OSSL="openssl enc -provider legacy -provider default -des-ecb -K 133457799BBCDFF1"
if $OSSL -nopad -in al.bin -out al.ossl 2>/dev/null; then
    cmp -s al.ct al.ossl && ok || bad "--nopad output differs from openssl -nopad"
    head -c 100003 /dev/urandom > big.bin
    "$DES" encrypt big.bin key.txt big.ct
    $OSSL -in big.bin -out big.ossl
    cmp -s big.ct big.ossl && ok || bad "padded output differs from openssl (PKCS#7)"
    $OSSL -d -in big.ct -out big.dec
    cmp -s big.bin big.dec && ok || bad "openssl cannot decrypt our padded output"
else
    echo "note: openssl legacy DES unavailable, cross-check skipped"
fi

# ---- wrong key is detected by the padding check (fixed inputs, deterministic)
printf 'hello world' > hw.bin
printf '0123456789ABCDEF' > key2.txt
"$DES" encrypt hw.bin key.txt hw.ct
"$DES" decrypt hw.ct key2.txt hw.out 2>err.txt; [ $? -eq 2 ] && grep -q "bad padding" err.txt && ok || bad "wrong key must fail with bad padding"

# ---- error paths: message on stderr and non-zero status ---------------------
errcase() { # description expected-status command...
    local d=$1 want=$2; shift 2
    "$@" >out.txt 2>err.txt; local rc=$?
    [ "$rc" = "$want" ] && [ -s err.txt ] && ok || bad "$d: rc=$rc (want $want), stderr='$(cat err.txt)'"
}
errcase "missing key file"     1 "$DES" encrypt al.bin /nonexistent/key.txt x.ct
errcase "missing input file"   1 "$DES" encrypt /nonexistent/in.bin key.txt x.ct
errcase "unwritable output"    1 "$DES" encrypt al.bin key.txt /nonexistent/dir/x.ct
errcase "no arguments"         2 "$DES"
errcase "three arguments"      2 "$DES" encrypt al.bin key.txt
errcase "bad verb"             2 "$DES" frobnicate al.bin key.txt x.ct
errcase "unknown option"       2 "$DES" encrypt al.bin key.txt x.ct --bogus
errcase "bad thread count"     2 "$DES" encrypt al.bin key.txt x.ct --threads 0
errcase "key file is a dir"    2 "$DES" encrypt al.bin /tmp x.ct
"$DES" --help >out.txt 2>&1 && grep -q usage out.txt && ok || bad "--help"

# ---- key file validation ----------------------------------------------------
keycase() { # description expected-status content
    local d=$1 want=$2; printf "$3" > kv.txt
    "$DES" encrypt al.bin kv.txt kv.ct --nopad >/dev/null 2>err.txt; local rc=$?
    [ "$rc" = "$want" ] || bad "$d: rc=$rc want $want ($(cat err.txt))"
    if [ "$want" = 0 ]; then cmp -s kv.ct al.ct && ok || bad "$d: ciphertext differs"; else ok; fi
}
keycase "empty key file"        2 ''
keycase "garbage key"           2 'hello'
keycase "8 hex digits"          2 '12345678'
keycase "17 hex digits"         2 '133457799BBCDFF10'
keycase "non-hex character"     2 '133457799BBCDFFG'
keycase "lowercase"             0 '133457799bbcdff1'
keycase "trailing newline"      0 '133457799BBCDFF1\n'
keycase "CRLF"                  0 '133457799BBCDFF1\r\n'
keycase "surrounding spaces"    0 '  133457799BBCDFF1  \n'
printf '0101010101010101' > weak.txt
"$DES" encrypt al.bin weak.txt w.ct 2>err.txt && grep -qi weak err.txt && ok || bad "weak key warning"

# ---- threads: any count gives identical output ------------------------------
head -c 300007 /dev/urandom > t.bin
"$DES" encrypt t.bin key.txt t1.ct --threads 1
for n in 2 3 7 64; do
    "$DES" encrypt t.bin key.txt tn.ct --threads $n
    cmp -s t1.ct tn.ct && ok || bad "--threads $n output differs from --threads 1"
done
"$DES" encrypt t.bin key.txt tn.ct --threads 1000 && cmp -s t1.ct tn.ct && ok || bad "threads > blocks"

# ---- hex output and timing --------------------------------------------------
head -c 262152 /dev/urandom > h.bin
"$DES" encrypt h.bin key.txt h.ct --hex h.hex
[ "$(wc -l < h.hex)" = $(( 262152 / 8 + 1 )) ] && ok || bad "hex line count $(wc -l < h.hex)"
[ "$(tail -n1 h.hex)" = "$(tail -c 8 h.ct | od -An -tx1 | tr -d ' \n' | tr a-f A-F)" ] && ok || bad "last hex line is not the last block in file order"
"$DES" encrypt h.bin key.txt h2.ct --time 2>err.txt && grep -q "MB/s" err.txt && ok || bad "--time output"

echo "tests: $pass passed, $fail failed"
[ "$fail" -eq 0 ]
