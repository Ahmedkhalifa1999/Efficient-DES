# Build:   make            -> ./des (optimised, warnings are errors)
# Test:    make test       -> unit tests + end-to-end CLI tests
#          make test-asan  -> the same under AddressSanitizer/UBSan
#          make test-tsan  -> the same under ThreadSanitizer
# Bench:   make bench      -> 64 MiB throughput, 1 thread vs all CPUs vs OpenSSL
CC       ?= cc
CFLAGS   ?= -O3
WARN      = -std=c11 -pedantic -Wall -Wextra -Wconversion -Wshadow -Wstrict-prototypes -Werror
CPPFLAGS += -Isrc
LDLIBS   += -pthread

SRC  = src/des.c src/main.c
HDR  = src/des.h

all: des

des: $(SRC) $(HDR)
	$(CC) $(CPPFLAGS) $(WARN) $(CFLAGS) -o $@ $(SRC) $(LDLIBS)

des-asan: $(SRC) $(HDR)
	$(CC) $(CPPFLAGS) $(WARN) -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -o $@ $(SRC) $(LDLIBS)

des-tsan: $(SRC) $(HDR)
	$(CC) $(CPPFLAGS) $(WARN) -O1 -g -fsanitize=thread -o $@ $(SRC) $(LDLIBS)

kat: tests/kat.c src/des.c $(HDR)
	$(CC) $(CPPFLAGS) $(WARN) $(CFLAGS) -o $@ tests/kat.c src/des.c

kat-asan: tests/kat.c src/des.c $(HDR)
	$(CC) $(CPPFLAGS) $(WARN) -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all -o $@ tests/kat.c src/des.c

test: des kat
	./kat
	tests/run_tests.sh ./des

test-asan: des-asan kat-asan
	./kat-asan
	tests/run_tests.sh ./des-asan

test-tsan: des-tsan
	tests/run_tests.sh ./des-tsan

bench: des
	tests/bench.sh ./des

format:
	clang-format -i $(SRC) $(HDR) tests/kat.c

clean:
	rm -f des des-asan des-tsan kat kat-asan

.PHONY: all test test-asan test-tsan bench format clean
