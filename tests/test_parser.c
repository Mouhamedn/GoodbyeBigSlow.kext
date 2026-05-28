#include <stdio.h>
#include <stdbool.h>
#include <string.h>

const char *mock_boot_args = "";

const char *PE_boot_args(void) {
    return mock_boot_args;
}

static bool is_arg_sep(char c)
{
    return c == ' ' || c == '\t' || c == '\0';
}

static bool has_boot_arg_flag(const char *key, const char *flag)
{
    const char *p = PE_boot_args();
    if (!p) return false;

    size_t key_len = strlen(key);
    size_t flag_len = strlen(flag);

    while (*p) {
        if (strncmp(p, key, key_len) == 0 && p[key_len] == '=') {
            const char *val = p + key_len + 1;
            const char *v = val;
            while (!is_arg_sep(*v)) {
                if (strncmp(v, flag, flag_len) == 0 &&
                    (v == val || v[-1] == ':') &&
                    (v[flag_len] == ':' || is_arg_sep(v[flag_len]))) {
                    return true;
                }
                v++;
            }
            p = v;
        } else {
            while (!is_arg_sep(*p)) p++;
        }
        while (*p == ' ' || *p == '\t') p++;
    }
    return false;
}

void test(const char *args, const char *key, const char *flag, bool expected) {
    mock_boot_args = args;
    bool result = has_boot_arg_flag(key, flag);
    if (result == expected) {
        printf("PASS: args=\"%s\", key=\"%s\", flag=\"%s\" => %s\n", args, key, flag, result ? "true" : "false");
    } else {
        printf("FAIL: args=\"%s\", key=\"%s\", flag=\"%s\" => expected %s, got %s\n",
               args, key, flag, expected ? "true" : "false", result ? "true" : "false");
    }
}

int main() {
    // Basic cases
    test("GoodbyeBigSlow=-turbo", "GoodbyeBigSlow", "-turbo", true);
    test("GoodbyeBigSlow=-speedstep", "GoodbyeBigSlow", "-speedstep", true);
    test("GoodbyeBigSlow=-turbo:-speedstep", "GoodbyeBigSlow", "-turbo", true);
    test("GoodbyeBigSlow=-turbo:-speedstep", "GoodbyeBigSlow", "-speedstep", true);
    test("GoodbyeBigSlow=-speedstep:-turbo", "GoodbyeBigSlow", "-turbo", true);

    // Multiple boot-args
    test("-v debug=0x100 GoodbyeBigSlow=-turbo", "GoodbyeBigSlow", "-turbo", true);
    test("GoodbyeBigSlow=-turbo -v debug=0x100", "GoodbyeBigSlow", "-turbo", true);
    test("-v GoodbyeBigSlow=-turbo debug=0x100", "GoodbyeBigSlow", "-turbo", true);

    // Flag in the middle
    test("GoodbyeBigSlow=foo:-turbo:bar", "GoodbyeBigSlow", "-turbo", true);

    // Key mismatch
    test("NotGoodbyeBigSlow=-turbo", "GoodbyeBigSlow", "-turbo", false);
    test("GoodbyeBigSlow2=-turbo", "GoodbyeBigSlow", "-turbo", false);

    // Flag mismatch (partial match)
    test("GoodbyeBigSlow=-turboboost", "GoodbyeBigSlow", "-turbo", false);
    test("GoodbyeBigSlow=anti-turbo", "GoodbyeBigSlow", "-turbo", false);

    // Empty/missing
    test("", "GoodbyeBigSlow", "-turbo", false);
    test("GoodbyeBigSlow=", "GoodbyeBigSlow", "-turbo", false);
    test("foo=bar", "GoodbyeBigSlow", "-turbo", false);

    // Multiple separators
    test("GoodbyeBigSlow=-turbo  -v", "GoodbyeBigSlow", "-turbo", true);
    test("  GoodbyeBigSlow=-turbo  ", "GoodbyeBigSlow", "-turbo", true);
    test("GoodbyeBigSlow=-turbo\t-v", "GoodbyeBigSlow", "-turbo", true);

    return 0;
}
