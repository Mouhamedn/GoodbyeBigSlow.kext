#include <stdio.h>
#include <string.h>
#include <stdbool.h>

const char *mock_boot_args = "";

const char *PE_boot_args(void) {
    return mock_boot_args;
}

static bool is_arg_sep(char c) {
    return c == ' ' || c == '\t' || c == '\0';
}

static bool has_boot_arg_flag(const char *key, const char *flag) {
    const char *p = PE_boot_args();
    if (p == NULL) {
        return false;
    }
    size_t key_len = strlen(key);
    size_t flag_len = strlen(flag);

    while (*p) {
        // Skip leading separators
        while (*p && (*p == ' ' || *p == '\t')) p++;
        if (!*p) break;

        // Check if this argument starts with "key="
        if (strncmp(p, key, key_len) == 0 && p[key_len] == '=') {
            const char *val = p + key_len + 1;
            // Find end of this boot-arg
            const char *end = val;
            while (*end && !is_arg_sep(*end)) end++;

            // Search for flag in val..end, separated by ':'
            const char *f = val;
            while (f < end) {
                const char *f_end = f;
                while (f_end < end && *f_end != ':') f_end++;

                if ((size_t)(f_end - f) == flag_len && strncmp(f, flag, flag_len) == 0) {
                    return true;
                }

                if (f_end < end) {
                    f = f_end + 1;
                } else {
                    break;
                }
            }
        }

        // Move to next argument
        while (*p && !is_arg_sep(*p)) p++;
    }
    return false;
}

void test(const char *args, const char *key, const char *flag, bool expected) {
    mock_boot_args = args;
    bool result = has_boot_arg_flag(key, flag);
    if (result == expected) {
        printf("PASS: args='%s', key='%s', flag='%s' => %s\n", args, key, flag, result ? "true" : "false");
    } else {
        printf("FAIL: args='%s', key='%s', flag='%s' => expected %s, got %s\n",
               args, key, flag, expected ? "true" : "false", result ? "true" : "false");
    }
}

int main() {
    test("GoodbyeBigSlow=-turbo", "GoodbyeBigSlow", "-turbo", true);
    test("GoodbyeBigSlow=-turbo:-speedstep", "GoodbyeBigSlow", "-turbo", true);
    test("GoodbyeBigSlow=-turbo:-speedstep", "GoodbyeBigSlow", "-speedstep", true);
    test("other=abc GoodbyeBigSlow=-speedstep", "GoodbyeBigSlow", "-speedstep", true);
    test("GoodbyeBigSlow=-speedstep other=abc", "GoodbyeBigSlow", "-speedstep", true);
    test("GoodbyeBigSlow=-speedstep:other", "GoodbyeBigSlow", "-speedstep", true);
    test("GoodbyeBigSlow=other:-speedstep", "GoodbyeBigSlow", "-speedstep", true);
    test("GoodbyeBigSlow=-speed", "GoodbyeBigSlow", "-speedstep", false);
    test("GoodbyeBigSlow=-speedstep2", "GoodbyeBigSlow", "-speedstep", false);
    test("GoodbyeBigSlow=-turbo", "GoodbyeBigSlow", "-speedstep", false);
    test("NotGoodbyeBigSlow=-speedstep", "GoodbyeBigSlow", "-speedstep", false);
    test("-v GoodbyeBigSlow=-turbo debug=0x1", "GoodbyeBigSlow", "-turbo", true);
    test("  GoodbyeBigSlow=-turbo  ", "GoodbyeBigSlow", "-turbo", true);
    test("GoodbyeBigSlow=-turbo:-speedstep:somethingelse", "GoodbyeBigSlow", "-speedstep", true);
    test("GoodbyeBigSlow=", "GoodbyeBigSlow", "-turbo", false);
    test("", "GoodbyeBigSlow", "-turbo", false);

    return 0;
}
