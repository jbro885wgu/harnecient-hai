/* hai_account - M2 local account. The ledger is the only authority (no ghost data).
 * Ledger row (append-only, one per account): CREATE|name|salt_hex|hash_hex|iter|unix
 *   Each row keeps its OWN iteration count, so raising pbkdf2_iter later never breaks old accounts.
 * Passphrase is hashed with PBKDF2-HMAC-SHA256 (OpenSSL) and a random 16-byte salt.
 * Usage:
 *   hai_account create <rules> <ledger> <name> <passphrase>
 *   hai_account login  <rules> <ledger> <name> <passphrase>
 * Known debt: the passphrase is on the command line (visible in ps) until the prompt verb exists (M2b).
 * Output: "OK create <name>" | "OK login <name>" | "REFUSED <reason>"
 * Exit: 0 ok, 1 refused, 2 usage, 9 missing rule. */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#define SALT_LEN 16
#define HASH_LEN 32

static long rule_value(const char *rules, const char *key, int *ok) {
    FILE *f = fopen(rules, "r");
    char line[512];
    *ok = 0;
    if (!f) return 0;
    while (fgets(line, sizeof line, f)) {
        char k[128], v[128];
        if (sscanf(line, "RULE | %127[^|] | %127s", k, v) == 2) {
            for (char *p = k; *p; p++) if (*p == ' ') { *p = 0; break; }
            if (strcmp(k, key) == 0) { *ok = 1; fclose(f); return atol(v); }
        }
    }
    fclose(f);
    return 0;
}

static void to_hex(const unsigned char *in, size_t n, char *out) {
    for (size_t i = 0; i < n; i++) sprintf(out + 2 * i, "%02x", in[i]);
    out[2 * n] = 0;
}

static int from_hex(const char *in, unsigned char *out, size_t n) {
    for (size_t i = 0; i < n; i++) {
        unsigned int b;
        if (sscanf(in + 2 * i, "%2x", &b) != 1) return 0;
        out[i] = (unsigned char)b;
    }
    return 1;
}

/* Scan the ledger for the CREATE row of name. */
static int find_account(const char *ledger, const char *name, char *salt, char *hash, long *iter) {
    FILE *f = fopen(ledger, "r");
    char line[1024];
    int found = 0;
    if (!f) return 0;
    while (fgets(line, sizeof line, f)) {
        char kind[16], nm[128], s[64], h[128];
        long it;
        if (sscanf(line, "%15[^|]|%127[^|]|%63[^|]|%127[^|]|%ld", kind, nm, s, h, &it) == 5 &&
            strcmp(kind, "CREATE") == 0 && strcmp(nm, name) == 0) {
            strcpy(salt, s); strcpy(hash, h); *iter = it; found = 1;
        }
    }
    fclose(f);
    return found;
}

static int derive(const char *pass, const unsigned char *salt, long iter, unsigned char *out) {
    return PKCS5_PBKDF2_HMAC(pass, (int)strlen(pass), salt, SALT_LEN, (int)iter,
                             EVP_sha256(), HASH_LEN, out);
}

static int name_ok(const char *name, long nmin, long nmax) {
    size_t n = strlen(name);
    if ((long)n < nmin || (long)n > nmax) return 0;
    for (size_t i = 0; i < n; i++) {
        char c = name[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-'))
            return 0;
    }
    return 1;
}

int main(int argc, char **argv) {
    if (argc != 6) {
        fprintf(stderr, "usage: hai_account create|login <rules> <ledger> <name> <passphrase>\n");
        return 2;
    }
    const char *verb = argv[1], *rules = argv[2], *ledger = argv[3], *name = argv[4], *pass = argv[5];
    int ok;
    long nmin = rule_value(rules, "name_min", &ok); if (!ok) { printf("REFUSED missing rule name_min\n"); return 9; }
    long nmax = rule_value(rules, "name_max", &ok); if (!ok) { printf("REFUSED missing rule name_max\n"); return 9; }
    long pmin = rule_value(rules, "pass_min", &ok); if (!ok) { printf("REFUSED missing rule pass_min\n"); return 9; }
    long pmax = rule_value(rules, "pass_max", &ok); if (!ok) { printf("REFUSED missing rule pass_max\n"); return 9; }
    long iters = rule_value(rules, "pbkdf2_iter", &ok); if (!ok) { printf("REFUSED missing rule pbkdf2_iter\n"); return 9; }

    if (!name_ok(name, nmin, nmax)) { printf("REFUSED name must be %ld-%ld letters, digits, _ or -\n", nmin, nmax); return 1; }
    if ((long)strlen(pass) < pmin || (long)strlen(pass) > pmax) { printf("REFUSED passphrase must be %ld-%ld characters\n", pmin, pmax); return 1; }

    char salt_s[64], hash_s[128];
    long stored_iter = 0;
    int exists = find_account(ledger, name, salt_s, hash_s, &stored_iter);

    if (strcmp(verb, "create") == 0) {
        if (exists) { printf("REFUSED name already taken\n"); return 1; }
        unsigned char salt[SALT_LEN], hash[HASH_LEN];
        if (RAND_bytes(salt, SALT_LEN) != 1) { printf("REFUSED no randomness\n"); return 1; }
        if (!derive(pass, salt, iters, hash)) { printf("REFUSED derive failed\n"); return 1; }
        char sh[2 * SALT_LEN + 1], hh[2 * HASH_LEN + 1];
        to_hex(salt, SALT_LEN, sh); to_hex(hash, HASH_LEN, hh);
        FILE *f = fopen(ledger, "a");
        if (!f) { printf("REFUSED cannot open ledger\n"); return 1; }
        fprintf(f, "CREATE|%s|%s|%s|%ld|%ld\n", name, sh, hh, iters, (long)time(NULL));
        fclose(f);
        printf("OK create %s\n", name);
        return 0;
    }
    if (strcmp(verb, "login") == 0) {
        if (!exists) { printf("REFUSED no such account\n"); return 1; }
        unsigned char salt[SALT_LEN], want[HASH_LEN], got[HASH_LEN];
        if (!from_hex(salt_s, salt, SALT_LEN) || !from_hex(hash_s, want, HASH_LEN)) { printf("REFUSED ledger row unreadable\n"); return 1; }
        if (!derive(pass, salt, stored_iter, got)) { printf("REFUSED derive failed\n"); return 1; }
        if (memcmp(got, want, HASH_LEN) != 0) { printf("REFUSED wrong passphrase\n"); return 1; }
        printf("OK login %s\n", name);
        return 0;
    }
    fprintf(stderr, "unknown verb %s\n", verb);
    return 2;
}
