#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

// Mock PE_boot_args
static const char *mock_boot_args = "";
char *PE_boot_args(void) {
    return (char *)mock_boot_args;
}

static bool isargsep(char c) {
    return c == ' ' || c == '\t';
}

bool my_parse_boot_argn(const char *arg_string, void *arg_ptr, int max_len) {
    char *args = PE_boot_args();
    if (!args || *args == '\0') return false;

    size_t key_len = strlen(arg_string);
    while (*args) {
        while (*args && isargsep(*args)) args++;
        if (*args == '\0') break;

        char *cp = args;
        while (*cp && !isargsep(*cp) && *cp != '=') cp++;

        size_t name_len = cp - args;
        if (name_len == key_len && strncmp(args, arg_string, name_len) == 0) {
            if (*cp == '=') {
                char *val = cp + 1;
                char *dest = (char *)arg_ptr;
                int i = 0;
                while (i < max_len - 1 && *val && !isargsep(*val)) {
                    dest[i++] = *val++;
                }
                if (max_len > 0) dest[i] = '\0';
                return true;
            } else {
                if (max_len > 0) {
                    memset(arg_ptr, 0, max_len);
                    if (max_len >= 8) {
                        *(int64_t *)arg_ptr = 1;
                    } else if (max_len >= 4) {
                        *(int32_t *)arg_ptr = 1;
                    } else if (max_len >= 2) {
                        *(int16_t *)arg_ptr = 1;
                    } else {
                        *(int8_t *)arg_ptr = 1;
                    }
                }
                return true;
            }
        }
        while (*args && !isargsep(*args) && *args != '=') args++;
        if (*args == '=') {
            while (*args && !isargsep(*args)) args++;
        }
    }
    return false;
}

static bool eql_flag(const char *a, const char *b, size_t n)
{
    while (n > 0 && *a && *b) {
        if (*a != *b || *a == ':' || *b == ':') return false;
        ++a; ++b; --n;
    }
    return n == 0;
}

static bool has_flag(const char *args, const char *arg)
{
    if (arg[0] == '-' || arg[0] == '+') {
        size_t n = strlen(arg);
        for (const char *p = args; *p; ++p) {
            if ((p == args || p[-1] == ':') && eql_flag(p, arg, n)
                    && (p[n] == 0 || p[n] == ':')) {
                return true;
            }
        }
    }
    return false;
}

void test(const char *boot_args_str) {
    mock_boot_args = boot_args_str;
    char buffer[18];
    memset(buffer, 0, sizeof(buffer));
    printf("Testing with boot-args: \"%s\"\n", boot_args_str);
    if (my_parse_boot_argn("GoodbyeBigSlow", buffer, sizeof(buffer))) {
        printf("  Found GoodbyeBigSlow\n");
        if (buffer[0] == '-' || buffer[0] == '+') {
            printf("  Buffer content (string): \"%s\"\n", buffer);
            if (has_flag(buffer, "-turbo")) printf("    Has -turbo\n");
            if (has_flag(buffer, "-speedstep")) printf("    Has -speedstep\n");
        } else {
            printf("  Buffer content (int): %d\n", (int)buffer[0]);
        }
    } else {
        printf("  GoodbyeBigSlow NOT found\n");
    }
    printf("\n");
}

int main() {
    test("GoodbyeBigSlow=-turbo:-speedstep");
    test("other=1 GoodbyeBigSlow=-speedstep debug=0x144");
    test("-v GoodbyeBigSlow=-turbo cpus=1");
    test("GoodbyeBigSlow");
    test("-GoodbyeBigSlow");
    test("Nothing here");
    test("GoodbyeBigSlow=v");
    test("GoodbyeBigSlow=verylongstringthatshouldbetruncated");
    test("  GoodbyeBigSlow=-turbo  ");
    test("GoodbyeBigSlow= ");
    test("GoodbyeBigSlow=");
    test("other=1 GoodbyeBigSlow");
    test("GoodbyeBigSlow other=1");
    return 0;
}
