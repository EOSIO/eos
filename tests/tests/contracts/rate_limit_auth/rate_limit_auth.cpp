/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eoslib/message.h>
#include <eoslib/types.hpp>
#include "rate_limit_auth.hpp"

extern "C" {
    void init()
    {
    }

    void test_auths2(const rate_limit_auth::Auths2& auth)
    {
       eos::print("auths2\n");
       requireAuth( auth.acct1 );
       requireAuth( auth.acct2 );
    }

    void test_auths3(const rate_limit_auth::Auths3& auth)
    {
       eos::print("auths3\n");
       requireAuth( auth.acct1 );
       requireAuth( auth.acct2 );
       requireAuth( auth.acct3 );
    }

    /// The apply method implements the dispatch of events to this contract
    void apply( uint64_t code, uint64_t action )
    {
       if( code == N(test1) )
       {
          if( action == N(auth2) )
          {
             test_auths2( eos::currentMessage< rate_limit_auth::Auths2 >() );
          }
          else if( action == N(auth3) )
          {
             test_auths3( eos::currentMessage< rate_limit_auth::Auths3 >() );
          }
       }
    }
}
