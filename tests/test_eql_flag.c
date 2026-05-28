#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

// Mocking kernel environment
#include "libkern/libkern.h"

// Pull in the implementation
// We need to define some things to make it compile with our mocks
#define __unused
#define __private_extern__
#include "../GoodbyeBigSlow/GoodbyeBigSlow.c"

void test_eql_flag() {
    // Basic matches
    assert(eql_flag("abc", "abc", 3) == true);
    assert(eql_flag("-turbo", "-turbo", 6) == true);

    // Partial matches (n is smaller than string length)
    assert(eql_flag("abcd", "abce", 3) == true);
    assert(eql_flag("abcd", "axcd", 1) == true);

    // Mismatches
    assert(eql_flag("abc", "abd", 3) == false);
    assert(eql_flag("abc", "abcd", 4) == false); // n=4, but "abc" ends
    assert(eql_flag("abcd", "abc", 4) == false); // n=4, but "abc" ends

    // Colon handling
    assert(eql_flag("abc:def", "abc:def", 7) == false); // hits ':' at index 3
    assert(eql_flag("abc", "ab:", 3) == false);
    assert(eql_flag("a:b", "a:b", 3) == false);

    // Length n = 0
    assert(eql_flag("abc", "def", 0) == true); // while (n > 0 ...) is skipped

    // Empty strings
    assert(eql_flag("", "", 0) == true);
    assert(eql_flag("", "", 1) == false);

    // n larger than string
    assert(eql_flag("abc", "abc", 5) == false); // while stops at *a && *b, then returns n == 0 (n is 2)

    printf("test_eql_flag passed!\n");
}

int main() {
    test_eql_flag();
    return 0;
}
