#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>

// Mock kernel functions and include the source file
#define static
// We need to provide these before including the source if they are extern there
void mp_rendezvous_no_intrs(void (*func)(void *), void *arg) {}
#include "../GoodbyeBigSlow.c"
#undef static

void test_eql_flag() {
    printf("Testing eql_flag...\n");
    assert(eql_flag("-turbo", "-turbo", 6));
    assert(eql_flag("-speedstep", "-speedstep", 10));
    assert(!eql_flag("-turbo", "-speedstep", 6));
    assert(!eql_flag("-turbo", "-turb", 6));
    assert(eql_flag("-turbo:other", "-turbo", 6));
    assert(!eql_flag("turbo", "-turbo", 6));
    printf("eql_flag tests passed!\n");
}

void test_has_flag() {
    printf("Testing has_flag...\n");

    // Single flag
    assert(has_flag("-turbo", "-turbo"));
    assert(!has_flag("-speedstep", "-turbo"));

    // Multiple flags
    assert(has_flag("-turbo:-speedstep", "-turbo"));
    assert(has_flag("-turbo:-speedstep", "-speedstep"));

    // Flag at the end
    assert(has_flag("other:-turbo", "-turbo"));

    // Flag in the middle
    assert(has_flag("a:-turbo:b", "-turbo"));

    // Edge cases
    assert(!has_flag("-turbos", "-turbo"));
    assert(!has_flag("not-turbo", "-turbo"));
    assert(!has_flag("-turbo", "turbo")); // has_flag expects '-' or '+' at start of arg

    // Colons
    assert(has_flag(":-turbo:", "-turbo"));

    printf("has_flag tests passed!\n");
}

int main() {
    test_eql_flag();
    test_has_flag();
    printf("All tests passed successfully!\n");
    return 0;
}
