#include "code_version_test.hpp"

void code_version_test::reqversion( name name, checksum256 version ) {
   check( get_code_version(name) == version, "code version doesn't match the expected value" );
}

void code_version_test::reqtime( name name, time_point last_updated ) {
   check( get_code_last_updated(name) == last_updated, "last updated time doesn't match the expected value" );
}
