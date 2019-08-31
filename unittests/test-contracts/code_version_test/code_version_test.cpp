#include "code_version_test.hpp"

using namespace eosio;

void code_version_test::reqversion( name name, checksum256 version ) {
   check( get_code_version(name) == version, "code version not match" );
}

void code_version_test::reqtime( name name, time_point last_code_update ) {
   check( get_code_last_update_time(name) == last_code_update, "last update time not match" );
}
