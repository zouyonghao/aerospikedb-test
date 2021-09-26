
#include "../test.h"

#include <aerospike/as_string.h>

/******************************************************************************
 * TEST CASES
 *****************************************************************************/

TEST( types_string_null, "as_string containing NULL" ) {
    as_string s;
    as_string_init(&s, NULL, false);
    assert( as_string_len(&s) == 0 );
    as_string_destroy(&s);
}

TEST( types_string_empty, "as_string containing \"\"" ) {
    as_string s;
    as_string_init(&s,"",false);
    assert( as_string_len(&s) == 0 );
    as_string_destroy(&s);
}

TEST( types_string_random, "as_string containing random values" ) {
    as_string s;
    as_string_init(&s,"dskghseoighweg",false);
    assert( as_string_len(&s) == 14 );
    as_string_destroy(&s);
}

/******************************************************************************
 * TEST SUITE
 *****************************************************************************/

SUITE( types_string, "as_string" ) {
    // suite_add( types_string_null );
    suite_add( types_string_empty );
    suite_add( types_string_random );
}