/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/types.hpp>

namespace eosiosystem {
   template <account_name SystemAccount>
   class native {
      public:
         ACTION(SystemAccount, newaccount) {
            EOSLIB_SERIALIZE(newaccount, BOOST_PP_SEQ_NIL)
         };

         static void on( const newaccount& ) {
         }

         ACTION( SystemAccount, updateauth ) {
            EOSLIB_SERIALIZE( updateauth, BOOST_PP_SEQ_NIL )
         };

         static void on( const updateauth& ) {
         }

         ACTION( SystemAccount, deleteauth ) {
            EOSLIB_SERIALIZE( deleteauth, BOOST_PP_SEQ_NIL )
         };

         static void on( const deleteauth& ) {
         }

         ACTION( SystemAccount, linkauth ) {
            EOSLIB_SERIALIZE( linkauth, BOOST_PP_SEQ_NIL )
         };

         static void on( const linkauth& ) {
         }
         
         ACTION( SystemAccount, unlinkauth ) {
            EOSLIB_SERIALIZE( unlinkauth, BOOST_PP_SEQ_NIL )
         };

         static void on( const unlinkauth& ) {
         }

         ACTION( SystemAccount, postrecovery ) {
            EOSLIB_SERIALIZE( postrecovery, BOOST_PP_SEQ_NIL )
         };

         static void on( const postrecovery& ) {
         }

         ACTION( SystemAccount, passrecovery ) {
            EOSLIB_SERIALIZE( passrecovery, BOOST_PP_SEQ_NIL )
         };

         static void on( const passrecovery& ) {
         }

         ACTION( SystemAccount, vetorecovery ) {
            EOSLIB_SERIALIZE( vetorecovery, BOOST_PP_SEQ_NIL )
         };

         static void on( const vetorecovery& ) {
         }

         ACTION( SystemAccount, setabi ) {
            EOSLIB_SERIALIZE( setabi, BOOST_PP_SEQ_NIL )
         };

         static void on( const setabi& ) {
         }

         ACTION( SystemAccount, onerror ) {
            EOSLIB_SERIALIZE( onerror, BOOST_PP_SEQ_NIL )
         };

         static void on( const onerror& ) {
         }
   };
}
