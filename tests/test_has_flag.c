#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../GoodbyeBigSlow/flag_utils.h"

void test_has_flag() {
    // Exact match
    assert(has_flag("-turbo", "-turbo") == true);
    assert(has_flag("+turbo", "+turbo") == true);

    // Multiple flags
    assert(has_flag("-turbo:-speedstep", "-turbo") == true);
    assert(has_flag("-turbo:-speedstep", "-speedstep") == true);
    assert(has_flag("-a:-b:-c", "-b") == true);

    // Non-matches
    assert(has_flag("-turbo", "-speedstep") == false);
    assert(has_flag("-speedstep", "-turbo") == false);

    // Partial matches (should fail)
    assert(has_flag("-turbos", "-turbo") == false);
    assert(has_flag("-turbo", "-turbos") == false);
    assert(has_flag("-t", "-turbo") == false);
    assert(has_flag("-turbo", "-t") == false);

    // Empty/Invalid inputs
    assert(has_flag("", "-turbo") == false);
    assert(has_flag("-turbo", "") == false);
    assert(has_flag("turbo", "turbo") == false); // Must start with - or +

    // Edge cases
    assert(has_flag("-turbo::", "-turbo") == true);
    assert(has_flag("::-turbo", "-turbo") == true);
    assert(has_flag(":-turbo:", "-turbo") == true);
    assert(has_flag("-turbo:", "-turbo") == true);
    assert(has_flag(":-turbo", "-turbo") == true);

    printf("All has_flag tests passed!\n");
}

int main() {
    test_has_flag();
    return 0;
}
