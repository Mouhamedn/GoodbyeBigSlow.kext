#include <stdio.h>
#include <stdbool.h>
#include <string.h>

static const char *mock_boot_args = "";

const char *PE_boot_args(void) {
    return mock_boot_args;
}

static inline bool is_arg_sep(char c) {
    return c == ' ' || c == '\t' || c == '\0';
}

static bool has_boot_arg_flag(const char *key, const char *flag) {
    const char *p = PE_boot_args();
    if (!p) return false;

    size_t key_len = strlen(key);
    size_t flag_len = strlen(flag);

    while (*p) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;

        if (strncmp(p, key, key_len) == 0 && p[key_len] == '=') {
            const char *v = p + key_len + 1;
            while (!is_arg_sep(*v)) {
                if (strncmp(v, flag, flag_len) == 0 && (is_arg_sep(v[flag_len]) || v[flag_len] == ':')) {
                    return true;
                }
                while (!is_arg_sep(*v) && *v != ':') v++;
                if (*v == ':') v++;
            }
        }

        while (!is_arg_sep(*p)) p++;
    }
    return false;
}

int main() {
    struct {
        const char *args;
        const char *key;
        const char *flag;
        bool expected;
    } test_cases[] = {
        {"GoodbyeBigSlow=-turbo", "GoodbyeBigSlow", "-turbo", true},
        {"GoodbyeBigSlow=-speedstep", "GoodbyeBigSlow", "-speedstep", true},
        {"GoodbyeBigSlow=-turbo:-speedstep", "GoodbyeBigSlow", "-turbo", true},
        {"GoodbyeBigSlow=-turbo:-speedstep", "GoodbyeBigSlow", "-speedstep", true},
        {"other=abc GoodbyeBigSlow=-turbo", "GoodbyeBigSlow", "-turbo", true},
        {"GoodbyeBigSlow=-turbo other=abc", "GoodbyeBigSlow", "-turbo", true},
        {"GoodbyeBigSlow=-turbo:-speedstep other=abc", "GoodbyeBigSlow", "-speedstep", true},
        {"GoodbyeBigSlow=-turbo:abc:-speedstep", "GoodbyeBigSlow", "-speedstep", true},
        {"GoodbyeBigSlow=-turbo", "GoodbyeBigSlow", "-speedstep", false},
        {"GoodbyeBigSlow=-turbostuff", "GoodbyeBigSlow", "-turbo", false},
        {"GoodbyeBigSlow=-turbo", "OtherKey", "-turbo", false},
        {"GoodbyeBigSlow=", "GoodbyeBigSlow", "-turbo", false},
        {"", "GoodbyeBigSlow", "-turbo", false},
        {"GoodbyeBigSlow=-turbo:-speedstep", "GoodbyeBigSlow", "-turbo", true},
        {"  GoodbyeBigSlow=-turbo  ", "GoodbyeBigSlow", "-turbo", true},
        {"GoodbyeBigSlow=-turbo: -speedstep", "GoodbyeBigSlow", "-speedstep", false}, // space breaks value
    };

    int failed = 0;
    for (int i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        mock_boot_args = test_cases[i].args;
        bool result = has_boot_arg_flag(test_cases[i].key, test_cases[i].flag);
        if (result != test_cases[i].expected) {
            printf("Test case %d failed: args=\"%s\", key=\"%s\", flag=\"%s\", expected=%d, got=%d\n",
                   i, test_cases[i].args, test_cases[i].key, test_cases[i].flag, test_cases[i].expected, result);
            failed++;
        }
    }

    if (failed == 0) {
        printf("All parser tests passed!\n");
        return 0;
    } else {
        printf("%d tests failed.\n", failed);
        return 1;
    }
}
