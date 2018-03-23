/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosiolib/types.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/math.hpp>
#include <eosiolib/multi_index.hpp>
#include <eosiolib/dispatcher.hpp>

struct contract {
   contract( account_name n ):_self(n){}
   account_name _self;
};



