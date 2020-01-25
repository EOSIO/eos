#pragma once

#include <eosio/testing/tester.hpp>

using namespace eosio::chain;
using namespace eosio::testing;

private_key_type get_private_key( name keyname, string role );

public_key_type  get_public_key( name keyname, string role );

void push_blocks( tester& from, tester& to, uint32_t block_num_limit = std::numeric_limits<uint32_t>::max() );

bool produce_until_transition( base_tester& t,
                               account_name last_producer,
                               account_name next_producer,
                               uint32_t max_num_blocks_to_produce = std::numeric_limits<uint32_t>::max() );

bool produce_until_blocks_from( base_tester& t,
                                const std::set<account_name>& expected_producers,
                                uint32_t max_num_blocks_to_produce = std::numeric_limits<uint32_t>::max() );
