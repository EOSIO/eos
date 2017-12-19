/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eos/egenesis/egenesis.hpp>

namespace eosio { namespace egenesis {

using namespace eosio::chain;

chain_id_type get_egenesis_chain_id()
{
   return chain_id_type();
}

void compute_egenesis_json( std::string& result )
{
   result = "";
}

fc::sha256 get_egenesis_json_hash()
{
   return fc::sha256::hash( "" );
}

} }
