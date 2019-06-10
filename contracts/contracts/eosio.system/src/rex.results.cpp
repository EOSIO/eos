#include <eosio.system/rex.results.hpp>

void rex_results::buyresult( const asset& rex_received ) { }

void rex_results::sellresult( const asset& proceeds ) { }

void rex_results::orderresult( const name& owner, const asset& proceeds ) { }

void rex_results::rentresult( const asset& rented_tokens ) { }

extern "C" void apply( uint64_t, uint64_t, uint64_t ) { }
