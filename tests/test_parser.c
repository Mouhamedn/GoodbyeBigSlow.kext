#include <stdio.h>
#include <string.h>
#include <stdbool.h>

static const char *mock_boot_args = "";

char *PE_boot_args(void) {
    return (char *)mock_boot_args;
}

static bool is_arg_sep(char c)
{
    return c == ' ' || c == '\t' || c == '\0';
}

static bool has_boot_arg_flag(const char *key, const char *flag)
{
    const char *args = PE_boot_args();
    if (!args) return false;

    size_t key_len = strlen(key);
    size_t flag_len = strlen(flag);

    while (*args) {
        while (*args == ' ' || *args == '\t') args++;
        if (!*args) break;

        if (strncmp(args, key, key_len) == 0 && args[key_len] == '=') {
            const char *v = args + key_len + 1;
            while (!is_arg_sep(*v)) {
                if ((v == args + key_len + 1 || v[-1] == ':') &&
                    strncmp(v, flag, flag_len) == 0 &&
                    (v[flag_len] == ':' || is_arg_sep(v[flag_len]))) {
                    return true;
                }
                v++;
            }
            args = v;
        } else {
            while (*args && !is_arg_sep(*args)) args++;
        }
    }
    return false;
}

void test(const char *boot_args, const char *key, const char *flag, bool expected) {
    mock_boot_args = boot_args;
    bool result = has_boot_arg_flag(key, flag);
    if (result == expected) {
        printf("PASS: boot-args=\"%s\", key=\"%s\", flag=\"%s\" => %s\n", boot_args, key, flag, result ? "true" : "false");
    } else {
        printf("FAIL: boot-args=\"%s\", key=\"%s\", flag=\"%s\" => expected %s, got %s\n",
               boot_args, key, flag, expected ? "true" : "false", result ? "true" : "false");
    }
}

int main() {
    test("GoodbyeBigSlow=-turbo", "GoodbyeBigSlow", "-turbo", true);
    test("GoodbyeBigSlow=-speedstep", "GoodbyeBigSlow", "-speedstep", true);
    test("GoodbyeBigSlow=-turbo:-speedstep", "GoodbyeBigSlow", "-turbo", true);
    test("GoodbyeBigSlow=-turbo:-speedstep", "GoodbyeBigSlow", "-speedstep", true);
    test("other=arg GoodbyeBigSlow=-turbo", "GoodbyeBigSlow", "-turbo", true);
    test("GoodbyeBigSlow=-turbo other=arg", "GoodbyeBigSlow", "-turbo", true);
    test("GoodbyeBigSlow=-turbo:other", "GoodbyeBigSlow", "-turbo", true);
    test("GoodbyeBigSlow=other:-turbo", "GoodbyeBigSlow", "-turbo", true);
    test("GoodbyeBigSlow=other:-turbo:another", "GoodbyeBigSlow", "-turbo", true);

    test("", "GoodbyeBigSlow", "-turbo", false);
    test("GoodbyeBigSlow=", "GoodbyeBigSlow", "-turbo", false);
    test("GoodbyeBigSlow=-turb", "GoodbyeBigSlow", "-turbo", false);
    test("GoodbyeBigSlow=-turbo-ext", "GoodbyeBigSlow", "-turbo", false);
    test("GoodbyeBigSlow=-turbo2", "GoodbyeBigSlow", "-turbo", false);
    test("NotGoodbyeBigSlow=-turbo", "GoodbyeBigSlow", "-turbo", false);
    test("GoodbyeBigSlow -turbo", "GoodbyeBigSlow", "-turbo", false);

    return 0;
}
