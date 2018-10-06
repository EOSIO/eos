/**
 *  @file
 *  @copyright defined in arisen/LICENSE.txt
 */
#pragma once

#include <arisenlib/eosio.hpp>
#include <arisenlib/token.hpp>
#include <arisenlib/reflect.hpp>
#include <arisenlib/generic_currency.hpp>

#include <bancor/converter.hpp>
#include <currency/currency.hpp>

namespace bancor {
   typedef arisen::generic_currency< arisen::token<N(other),S(4,OTHER)> >  other_currency;
   typedef arisen::generic_currency< arisen::token<N(bancor),S(4,RELAY)> > relay_currency;
   typedef arisen::generic_currency< arisen::token<N(currency),S(4,CUR)> > cur_currency;

   typedef converter<relay_currency, other_currency, cur_currency > example_converter;
} /// bancor

