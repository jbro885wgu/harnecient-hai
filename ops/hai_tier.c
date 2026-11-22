/* hai_tier - decides whether a user's chain may be published, and at what price (millicones).
 *
 *   hai_tier check <tier_rules.pdl> <free|paid> <public|private>
 *       Prints `OK tier=<t> visibility=<v> price_millicones=<p>` or `REFUSED <reason>`.
 *       Exit 0 allowed | 2 setup (unknown tier, missing rule) | 4 refused (wrong visibility for the tier).
 *   hai_tier pack  <tier_rules.pdl> <free|paid> <ledger.txt> <outdir>
 *       Writes <outdir>/pack.pdl (PACK row with the ledger's SHA-256 and row count) and <outdir>/ledger.txt.
 *       Only a PUBLIC tier may be packed; a private chain writes NOTHING (exit 4). A pack is a projection of the ledger.
 *
 * Visibility and price are RULE rows (never in this file). The ledger stays the only authority: a pack is a copy.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <openssl/evp.h>

static int rule(const char *path, const char *key, char *out, size_t n) {
    FILE *f = fopen(path, "r"); if (!f) return 0;
    char line[512]; int found = 0;
    while (fgets(line, sizeof line, f)) {
        if (line[0] == '#' || strncmp(line, "RULE", 4) != 0) continue;
        char *p = strchr(line, '|'); if (!p) continue; p++;
        char *k = p; while (*k == ' ') k++;
        char *e = k; while (*e && *e != ' ' && *e != '|') e++;
        size_t kl = (size_t)(e - k);
        if (kl != strlen(key) || strncmp(k, key, kl) != 0) continue;
        char *v = strchr(e, '|'); if (!v) continue; v++;
        while (*v == ' ') v++;
        char *ve = v + strlen(v); while (ve > v && (ve[-1] == '\n' || ve[-1] == ' ' || ve[-1] == '\r')) ve--;
        snprintf(out, n, "%.*s", (int)(ve - v), v); found = 1;
    }
    fclose(f);
    return found;
}

static int decide(const char *rules, const char *tier, const char *vis, long *price) {
    char key[64], want[32], pr[32];
    if (strcmp(tier, "free") != 0 && strcmp(tier, "paid") != 0) { printf("REFUSED unknown tier %s\n", tier); return 2; }
    snprintf(key, sizeof key, "tier_%s_visibility", tier);
    if (!rule(rules, key, want, sizeof want)) { printf("REFUSED missing rule %s\n", key); return 2; }
    snprintf(key, sizeof key, "tier_%s_price_millicones", tier);
    if (!rule(rules, key, pr, sizeof pr)) { printf("REFUSED missing rule %s\n", key); return 2; }
    *price = atol(pr);
    if (strcmp(vis, want) != 0) { printf("REFUSED tier %s requires %s visibility (asked %s)\n", tier, want, vis); return 4; }
    printf("OK tier=%s visibility=%s price_millicones=%ld\n", tier, vis, *price);
    return 0;
}

static int sha256_file(const char *path, char hex[65], long *rows) {
    FILE *f = fopen(path, "rb"); if (!f) return 0;
    EVP_MD_CTX *c = EVP_MD_CTX_new(); EVP_DigestInit_ex(c, EVP_sha256(), NULL);
    unsigned char buf[4096]; size_t n; *rows = 0;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0) {
        EVP_DigestUpdate(c, buf, n);
        for (size_t i = 0; i < n; i++) if (buf[i] == '\n') (*rows)++;
    }
    unsigned char md[32]; unsigned int ml = 0;
    EVP_DigestFinal_ex(c, md, &ml); EVP_MD_CTX_free(c); fclose(f);
    for (int i = 0; i < 32; i++) sprintf(hex + 2 * i, "%02x", md[i]);
    return 1;
}

int main(int argc, char **argv) {
    if (argc == 5 && strcmp(argv[1], "check") == 0) {
        long price = 0;
        return decide(argv[2], argv[3], argv[4], &price);
    }
    if (argc == 6 && strcmp(argv[1], "pack") == 0) {
        long price = 0;
        int rc = decide(argv[2], argv[3], "public", &price);
        if (rc != 0) {
            if (rc == 4) { printf("NOT PACKED: the tier is private, nothing written\n"); }
            return rc == 4 ? 4 : rc;
        }
        char hex[65]; long rows = 0;
        if (!sha256_file(argv[4], hex, &rows)) { printf("REFUSED cannot read ledger %s\n", argv[4]); return 2; }
        mkdir(argv[5], 0755);
        char dst[1024], cmd[2200];
        snprintf(dst, sizeof dst, "%s/ledger.txt", argv[5]);
        snprintf(cmd, sizeof cmd, "cp '%s' '%s'", argv[4], dst);
        if (system(cmd) != 0) { printf("REFUSED copy failed\n"); return 3; }
        snprintf(dst, sizeof dst, "%s/pack.pdl", argv[5]);
        FILE *f = fopen(dst, "w"); if (!f) { printf("REFUSED cannot write %s\n", dst); return 3; }
        fprintf(f, "# pack.pdl - a public copy of a user's ledger (hai_tier pack). The ledger is the authority; this is a projection.\n");
        fprintf(f, "PACK | tier=%s | visibility=public | price_millicones=%ld | rows=%ld | sha256=%s | ledger=ledger.txt\n", argv[3], price, rows, hex);
        fclose(f);
        printf("PACKED tier=%s rows=%ld sha256=%s\n", argv[3], rows, hex);
        return 0;
    }
    fprintf(stderr, "usage: hai_tier check <tier_rules.pdl> <free|paid> <public|private>\n       hai_tier pack <tier_rules.pdl> <free|paid> <ledger.txt> <outdir>\n");
    return 2;
}
