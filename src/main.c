/* main.c - command-line front end: DES-ECB file encryption with PKCS#7
 * padding, multi-threaded over a contiguous block partition.
 *
 *   des encrypt <input> <keyfile> <output> [options]
 *   des decrypt <input> <keyfile> <output> [options]
 */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "des.h"

enum { EXIT_IO_ERROR = 1, EXIT_USAGE = 2 };

#define KEY_HEX_DIGITS 16

struct options {
    int decrypt;
    const char *input;
    const char *keyfile;
    const char *output;
    const char *hexfile;
    int nopad;
    int show_time;
    long threads;
};

struct job {
    const des_key_schedule *ks;
    uint8_t *buf;
    size_t first_block;
    size_t end_block;
    int decrypt;
};

static void usage(FILE *stream) {
    fputs("usage: des encrypt|decrypt <input> <keyfile> <output> [options]\n"
          "\n"
          "  <keyfile>      text file holding the 64-bit key as 16 hex digits\n"
          "\n"
          "options:\n"
          "  --nopad        raw mode: no PKCS#7 padding; input must be a multiple of 8 bytes\n"
          "  --threads N    worker threads (default: number of online CPUs)\n"
          "  --time         print elapsed wall time and throughput to stderr\n"
          "  --hex FILE     also write every processed block as a 16-digit hex line to FILE\n"
          "  -h, --help     show this help\n"
          "\n"
          "exit status: 0 success, 1 I/O error, 2 usage, key or padding error\n"
          "\n"
          "DES is an obsolete cipher and ECB mode leaks patterns; educational use only.\n",
          stream);
}

static void die_errno(const char *path) {
    fprintf(stderr, "des: %s: %s\n", path, strerror(errno));
    exit(EXIT_IO_ERROR);
}

static void die_usage(const char *msg) {
    fprintf(stderr, "des: %s\n", msg);
    exit(EXIT_USAGE);
}

static int parse_args(int argc, char **argv, struct options *opt) {
    const char *positional[3];
    int npos = 0;

    memset(opt, 0, sizeof *opt);
    opt->threads = -1;

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (strcmp(a, "-h") == 0 || strcmp(a, "--help") == 0) {
            usage(stdout);
            exit(0);
        } else if (strcmp(a, "--nopad") == 0) {
            opt->nopad = 1;
        } else if (strcmp(a, "--time") == 0) {
            opt->show_time = 1;
        } else if (strcmp(a, "--threads") == 0 || strcmp(a, "-t") == 0) {
            char *end;
            if (++i >= argc) {
                die_usage("--threads needs a value");
            }
            errno = 0;
            opt->threads = strtol(argv[i], &end, 10);
            if (errno != 0 || *end != '\0' || opt->threads < 1 || opt->threads > 1024) {
                die_usage("--threads must be an integer between 1 and 1024");
            }
        } else if (strcmp(a, "--hex") == 0) {
            if (++i >= argc) {
                die_usage("--hex needs a file name");
            }
            opt->hexfile = argv[i];
        } else if (a[0] == '-' && a[1] != '\0') {
            fprintf(stderr, "des: unknown option '%s'\n", a);
            return -1;
        } else if (npos < 4) {
            if (npos == 0) {
                if (strcmp(a, "encrypt") == 0) {
                    opt->decrypt = 0;
                } else if (strcmp(a, "decrypt") == 0) {
                    opt->decrypt = 1;
                } else {
                    fprintf(stderr, "des: first argument must be 'encrypt' or 'decrypt'\n");
                    return -1;
                }
            } else {
                positional[npos - 1] = a;
            }
            npos++;
        } else {
            fprintf(stderr, "des: too many arguments\n");
            return -1;
        }
    }
    if (npos != 4) {
        fprintf(stderr, "des: expected <input> <keyfile> <output>\n");
        return -1;
    }
    opt->input = positional[0];
    opt->keyfile = positional[1];
    opt->output = positional[2];
    return 0;
}

/* Read the key file: exactly 16 hex digits, surrounding whitespace allowed. */
static uint64_t read_key(const char *path) {
    char line[128];
    char *s;
    char *end;
    size_t n;
    uint64_t key;
    FILE *f = fopen(path, "r");

    if (f == NULL) {
        die_errno(path);
    }
    if (fgets(line, sizeof line, f) == NULL) {
        fclose(f);
        fprintf(stderr, "des: %s: key file is empty\n", path);
        exit(EXIT_USAGE);
    }
    fclose(f);

    s = line;
    while (*s == ' ' || *s == '\t') {
        s++;
    }
    n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r' || s[n - 1] == ' ' || s[n - 1] == '\t')) {
        s[--n] = '\0';
    }
    if (n != KEY_HEX_DIGITS || strspn(s, "0123456789abcdefABCDEF") != KEY_HEX_DIGITS) {
        fprintf(stderr, "des: %s: key file must contain exactly %d hex digits\n", path,
                KEY_HEX_DIGITS);
        exit(EXIT_USAGE);
    }
    key = strtoull(s, &end, 16);
    if (des_is_weak_key(key)) {
        fprintf(stderr, "des: warning: %s holds a weak or semi-weak DES key\n", path);
    }
    return key;
}

/* Read a whole file into memory, leaving `spare` writable bytes after it. */
static uint8_t *read_file(const char *path, size_t spare, size_t *len_out) {
    FILE *f = fopen(path, "rb");
    struct stat st;
    size_t cap = 1u << 16;
    size_t len = 0;
    uint8_t *buf;

    if (f == NULL) {
        die_errno(path);
    }
    if (fstat(fileno(f), &st) == 0 && S_ISREG(st.st_mode) && st.st_size > 0) {
        cap = (size_t)st.st_size;
    }
    buf = malloc(cap + spare);
    if (buf == NULL) {
        fprintf(stderr, "des: out of memory\n");
        exit(EXIT_IO_ERROR);
    }
    for (;;) {
        size_t got;
        if (len == cap) {
            uint8_t *bigger;
            cap *= 2;
            bigger = realloc(buf, cap + spare);
            if (bigger == NULL) {
                fprintf(stderr, "des: out of memory\n");
                exit(EXIT_IO_ERROR);
            }
            buf = bigger;
        }
        got = fread(buf + len, 1, cap - len, f);
        len += got;
        if (got == 0) {
            if (ferror(f)) {
                die_errno(path);
            }
            break;
        }
    }
    fclose(f);
    *len_out = len;
    return buf;
}

static void write_file(const char *path, const uint8_t *buf, size_t len) {
    FILE *f = fopen(path, "wb");
    if (f == NULL) {
        die_errno(path);
    }
    if (len > 0 && fwrite(buf, 1, len, f) != len) {
        die_errno(path);
    }
    if (fclose(f) != 0) {
        die_errno(path);
    }
}

static void write_hex(const char *path, const uint8_t *buf, size_t nblocks) {
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        die_errno(path);
    }
    for (size_t i = 0; i < nblocks; i++) {
        const uint8_t *b = buf + i * DES_BLOCK_BYTES;
        fprintf(f, "%02X%02X%02X%02X%02X%02X%02X%02X\n", b[0], b[1], b[2], b[3], b[4], b[5], b[6],
                b[7]);
    }
    if (fclose(f) != 0) {
        die_errno(path);
    }
}

static void *worker(void *arg) {
    const struct job *j = arg;
    for (size_t i = j->first_block; i < j->end_block; i++) {
        uint8_t *p = j->buf + i * DES_BLOCK_BYTES;
        uint64_t x = des_load_be64(p);
        x = j->decrypt ? des_decrypt_block(j->ks, x) : des_encrypt_block(j->ks, x);
        des_store_be64(p, x);
    }
    return NULL;
}

static long default_threads(void) {
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return n < 1 ? 1 : n;
}

/* Process nblocks blocks in place, split into contiguous ranges, one per thread. */
static void process_blocks(const des_key_schedule *ks, uint8_t *buf, size_t nblocks, int decrypt,
                           long threads) {
    size_t nthreads;
    size_t per;
    size_t rem;
    size_t next = 0;
    pthread_t *tids;
    struct job *jobs;

    if (nblocks == 0) {
        return;
    }
    nthreads = threads < 1 ? 1 : (size_t)threads;
    if (nthreads > nblocks) {
        nthreads = nblocks;
    }
    if (nthreads == 1) {
        struct job j = {ks, buf, 0, nblocks, decrypt};
        worker(&j);
        return;
    }
    tids = malloc(nthreads * sizeof *tids);
    jobs = malloc(nthreads * sizeof *jobs);
    if (tids == NULL || jobs == NULL) {
        fprintf(stderr, "des: out of memory\n");
        exit(EXIT_IO_ERROR);
    }
    per = nblocks / nthreads;
    rem = nblocks % nthreads;
    for (size_t t = 0; t < nthreads; t++) {
        int rc;
        jobs[t].ks = ks;
        jobs[t].buf = buf;
        jobs[t].decrypt = decrypt;
        jobs[t].first_block = next;
        next += per + (t < rem ? 1 : 0);
        jobs[t].end_block = next;
        rc = pthread_create(&tids[t], NULL, worker, &jobs[t]);
        if (rc != 0) {
            fprintf(stderr, "des: pthread_create: %s\n", strerror(rc));
            exit(EXIT_IO_ERROR);
        }
    }
    for (size_t t = 0; t < nthreads; t++) {
        pthread_join(tids[t], NULL);
    }
    free(jobs);
    free(tids);
}

static double now_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

int main(int argc, char **argv) {
    struct options opt;
    des_key_schedule ks;
    uint8_t *buf;
    size_t len;
    size_t out_len;
    size_t nblocks;
    double t0;

    if (parse_args(argc, argv, &opt) != 0) {
        usage(stderr);
        return EXIT_USAGE;
    }
    if (opt.threads < 0) {
        opt.threads = default_threads();
    }

    t0 = now_seconds();
    des_set_key(&ks, read_key(opt.keyfile));
    buf = read_file(opt.input, DES_BLOCK_BYTES, &len);

    if (opt.decrypt) {
        if (len % DES_BLOCK_BYTES != 0) {
            fprintf(stderr, "des: %s: size %zu is not a multiple of %d bytes\n", opt.input, len,
                    DES_BLOCK_BYTES);
            goto fail_usage;
        }
        if (!opt.nopad && len == 0) {
            fprintf(stderr, "des: %s: empty ciphertext (expected at least one padded block)\n",
                    opt.input);
            goto fail_usage;
        }
        out_len = len;
    } else if (opt.nopad) {
        if (len % DES_BLOCK_BYTES != 0) {
            fprintf(
                stderr,
                "des: %s: size %zu is not a multiple of %d bytes; drop --nopad to use padding\n",
                opt.input, len, DES_BLOCK_BYTES);
            goto fail_usage;
        }
        out_len = len;
    } else {
        /* PKCS#7: always append 1..8 bytes, each holding the pad length. */
        size_t pad = DES_BLOCK_BYTES - (len % DES_BLOCK_BYTES);
        memset(buf + len, (int)pad, pad);
        out_len = len + pad;
    }

    nblocks = out_len / DES_BLOCK_BYTES;
    process_blocks(&ks, buf, nblocks, opt.decrypt, opt.threads);

    if (opt.decrypt && !opt.nopad) {
        size_t pad = buf[out_len - 1];
        int ok = pad >= 1 && pad <= DES_BLOCK_BYTES && pad <= out_len;
        for (size_t i = 0; ok && i < pad; i++) {
            ok = buf[out_len - 1 - i] == pad;
        }
        if (!ok) {
            fprintf(stderr, "des: %s: bad padding (wrong key or corrupt input)\n", opt.input);
            goto fail_usage;
        }
        out_len -= pad;
    }

    write_file(opt.output, buf, out_len);
    if (opt.hexfile != NULL) {
        write_hex(opt.hexfile, buf, nblocks);
    }
    if (opt.show_time) {
        double dt = now_seconds() - t0;
        fprintf(stderr, "des: %zu bytes in %.3f s (%.1f MB/s, %ld thread%s)\n", out_len, dt,
                dt > 0 ? (double)out_len / dt / 1e6 : 0.0, opt.threads,
                opt.threads == 1 ? "" : "s");
    }
    free(buf);
    return 0;

fail_usage:
    free(buf);
    return EXIT_USAGE;
}
