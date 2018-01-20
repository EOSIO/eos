#include "eosioclient.hpp"

eosio::chain_apis::read_only::get_info_results Eosioclient::get_info()
{
    return call(host, port, get_info_func ).as<eosio::chain_apis::read_only::get_info_results>();
}
