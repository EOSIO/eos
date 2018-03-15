/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/eosio.hpp>
#include <eosiolib/fixed_key.hpp>
#include <eosiolib/token.hpp>
#include <eosiolib/transaction.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/db.hpp>
#include <eosiolib/reflect.hpp>
#include <eosiolib/print.hpp>

#include <eosiolib/generic_currency.hpp>
#include <eosiolib/datastream.hpp>
#include <eosiolib/serialize.hpp>
#include <eosiolib/multi_index.hpp>
#include <eosiolib/privileged.h>

#include <array>
#include <algorithm>
#include <map>

namespace eosiosystem {
   using eosio::indexed_by;
   using eosio::const_mem_fun;
   using eosio::bytes;
   using std::map;
   using std::pair;
   using eosio::print;

   template<account_name SystemAccount>
   class contract {
      public:
         static const account_name system_account = SystemAccount;
         typedef eosio::generic_currency< eosio::token<system_account,S(4,EOS)> > currency;
         typedef typename currency::token_type system_token_type;
         using key256 = eosio::key256;

         struct producer_votes {
            account_name      owner;
            uint64_t          padding = 0;
            uint128_t         total_votes;

            uint64_t    primary_key()const { return owner;       }
            uint128_t   by_votes()const    { return total_votes; }

            EOSLIB_SERIALIZE( producer_votes, (owner)(total_votes) )
         };
         typedef eosio::multi_index< N(producervote), producer_votes,
            indexed_by<N(prototalvote), const_mem_fun<producer_votes, uint128_t, &producer_votes::by_votes>  >
         >  producer_votes_index_type;

         struct account_votes {
            account_name                owner;
            account_name                proxy;
            uint32_t                    last_update;
            system_token_type           staked;
            std::vector<account_name>   producers;

            uint64_t primary_key()const { return owner; }

            EOSLIB_SERIALIZE( account_votes, (owner)(proxy)(last_update)(staked)(producers) )
         };
         typedef eosio::multi_index< N(accountvotes), account_votes>  account_votes_index_type;


         struct producer_config {
            account_name      owner;
            eosio::bytes      packed_key; /// a packed public key object

            uint64_t primary_key()const { return owner;       }
            EOSLIB_SERIALIZE( producer_config, (owner)(packed_key) )
         };
         typedef eosio::multi_index< N(producercfg), producer_config>  producer_config_index_type;

         struct total_resources {
            account_name owner;
            typename currency::token_type total_net_weight;
            typename currency::token_type total_cpu_weight;
            uint32_t total_ram = 0;

            uint64_t primary_key()const { return owner; }

            EOSLIB_SERIALIZE( total_resources, (owner)(total_net_weight)(total_cpu_weight)(total_ram) )
         };


         /**
          *  Every user 'from' has a scope/table that uses every receipient 'to' as the primary key.
          */
         struct delegated_bandwidth {
            account_name from;
            account_name to;
            typename currency::token_type net_weight;
            typename currency::token_type cpu_weight;

            uint32_t start_pending_net_withdraw = 0;
            typename currency::token_type pending_net_withdraw;
            uint64_t deferred_net_withdraw_handler = 0;

            uint32_t start_pending_cpu_withdraw = 0;
            typename currency::token_type pending_cpu_withdraw;
            uint64_t deferred_cpu_withdraw_handler = 0;


            uint64_t  primary_key()const { return to; }

            EOSLIB_SERIALIZE( delegated_bandwidth, (from)(to)(net_weight)(cpu_weight)
                              (start_pending_net_withdraw)(pending_net_withdraw)(deferred_net_withdraw_handler)
                              (start_pending_cpu_withdraw)(pending_cpu_withdraw)(deferred_cpu_withdraw_handler) )

         };


         typedef eosio::multi_index< N(totalband), total_resources>  total_resources_index_type;
         typedef eosio::multi_index< N(delband), delegated_bandwidth> del_bandwidth_index_type;

         struct genesis_balance {
            uint64_t                       id; // required by multi_index, auto incremented
            key256                         pubkey_hash;
            typename currency::token_type  balance;

            uint64_t primary_key()const { return id; }
            const key256& get_pubkey_hash()const { return pubkey_hash; }

            EOSLIB_SERIALIZE( genesis_balance, (id)(pubkey_hash)(balance) )
         };
         typedef eosio::multi_index< N(genbal), genesis_balance,
               indexed_by<N(genpubkey), const_mem_fun<genesis_balance, const key256&, &genesis_balance::get_pubkey_hash> >
         > genesis_balance_index_type;


         ACTION( SystemAccount, finishundel ) {
            account_name from;
            account_name to;

            EOSLIB_SERIALIZE( finishundel, (from)(to) )
         };

         ACTION( SystemAccount, regproducer ) {
            account_name producer;
            bytes        producer_key;

            EOSLIB_SERIALIZE( regproducer, (producer)(producer_key) )
         };

         ACTION( SystemAccount, regproxy ) {
            account_name proxy_to_register;

            EOSLIB_SERIALIZE( regproxy, (proxy_to_register) )
         };

         ACTION( SystemAccount, delegatebw ) {
            account_name                    from;
            account_name                    receiver;
            typename currency::token_type   stake_net_quantity;
            typename currency::token_type   stake_cpu_quantity;


            EOSLIB_SERIALIZE( delegatebw, (from)(receiver)(stake_net_quantity)(stake_cpu_quantity) )
         };

         ACTION( SystemAccount, undelegatebw ) {
            account_name                    from;
            account_name                    receiver;
            typename currency::token_type   unstake_net_quantity;
            typename currency::token_type   unstake_cpu_quantity;

            EOSLIB_SERIALIZE( undelegatebw, (from)(receiver)(unstake_net_quantity)(unstake_cpu_quantity) )
         };

         ACTION( SystemAccount, setgen ) {
            std::map<checksum256, typename currency::token_type> genesis_balances;

            EOSLIB_SERIALIZE( setgen, (genesis_balances) )
         };

         ACTION( SystemAccount, claimgen ) {
            account_name   name;
            checksum256    pubkey_hash;
            uint32_t       cfa_idx; // context free action index

            EOSLIB_SERIALIZE( claimgen, (name)(pubkey_hash)(cfa_idx) )
         };

         // context free action
         ACTION( SystemAccount, keyproof ) {
            checksum256    pubkey_hash;
            checksum256    digest;  // hash of purchase agreement
            uint32_t       cfd_idx; // context free data index

            EOSLIB_SERIALIZE( keyproof, (pubkey_hash)(digest)(cfd_idx) )
         };

         ACTION( SystemAccount, nonce ) {
            eosio::string                   value;

            EOSLIB_SERIALIZE( nonce, (value) )
         };

         /// new id options:
         //  1. hash + collision
         //  2. incrementing count  (key=> tablename

         static void on( const delegatebw& del ) {
            eosio_assert( del.stake_cpu_quantity.quantity >= 0, "must stake a positive amount" );
            eosio_assert( del.stake_net_quantity.quantity >= 0, "must stake a positive amount" );

            auto total_stake = del.stake_cpu_quantity + del.stake_net_quantity;
            eosio_assert( total_stake.quantity >= 0, "must stake a positive amount" );


            require_auth( del.from );

            del_bandwidth_index_type     del_index( SystemAccount, del.from );
            total_resources_index_type   total_index( SystemAccount, del.receiver );

            //eosio_assert( is_account( del.receiver ), "can only delegate resources to an existing account" );

            auto itr = del_index.find( del.receiver);
            if( itr != del_index.end() ) {
               del_index.emplace( del.from, [&]( auto& dbo ){
                  dbo.from       = del.from;
                  dbo.to         = del.receiver;
                  dbo.net_weight = del.stake_net_quantity;
                  dbo.cpu_weight = del.stake_cpu_quantity;
               });
            }
            else {
               del_index.modify( itr, del.from, [&]( auto& dbo ){
                  dbo.net_weight = del.stake_net_quantity;
                  dbo.cpu_weight = del.stake_cpu_quantity;
               });
            }

            auto tot_itr = total_index.find( del.receiver );
            if( tot_itr == total_index.end() ) {
               tot_itr = total_index.emplace( del.from, [&]( auto& tot ) {
                  tot.owner = del.receiver;
                  tot.total_net_weight += del.stake_net_quantity;
                  tot.total_cpu_weight += del.stake_cpu_quantity;
               });
            } else {
               total_index.modify( tot_itr, 0, [&]( auto& tot ) {
                  tot.total_net_weight += del.stake_net_quantity;
                  tot.total_cpu_weight += del.stake_cpu_quantity;
               });
            }

            set_resource_limits( tot_itr->owner, tot_itr->total_ram, tot_itr->total_net_weight.quantity, tot_itr->total_cpu_weight.quantity, 0 );

            currency::inline_transfer( del.from, SystemAccount, total_stake, "stake bandwidth" );
         } // delegatebw



         static void on( const undelegatebw& del ) {
            eosio_assert( del.unstake_cpu_quantity.quantity >= 0, "must stake a positive amount" );
            eosio_assert( del.unstake_net_quantity.quantity >= 0, "must stake a positive amount" );

            auto total_stake = del.unstake_cpu_quantity + del.unstake_net_quantity;
            eosio_assert( total_stake.quantity >= 0, "must stake a positive amount" );

            require_auth( del.from );

            del_bandwidth_index_type     del_index( SystemAccount, del.from );
            total_resources_index_type   total_index( SystemAccount, del.receiver );

            //eosio_assert( is_account( del.receiver ), "can only delegate resources to an existing account" );

            const auto& dbw = del_index.get(del.receiver);
            eosio_assert( dbw.net_weight >= del.unstake_net_quantity, "insufficient staked net bandwidth" );
            eosio_assert( dbw.cpu_weight >= del.unstake_cpu_quantity, "insufficient staked cpu bandwidth" );

            del_index.modify( dbw, del.from, [&]( auto& dbo ){
               dbo.net_weight -= del.unstake_net_quantity;
               dbo.cpu_weight -= del.unstake_cpu_quantity;

            });

            const auto& totals = total_index.get( del.receiver );
            total_index.modify( totals, 0, [&]( auto& tot ) {
               tot.total_net_weight -= del.unstake_net_quantity;
               tot.total_cpu_weight -= del.unstake_cpu_quantity;
            });

            set_resource_limits( totals.owner, totals.total_ram, totals.total_net_weight.quantity, totals.total_cpu_weight.quantity, 0 );

            /// TODO: implement / enforce time delays on withdrawing
            currency::inline_transfer( SystemAccount, del.from, total_stake, "unstake bandwidth" );
         } // undelegatebw





         /**
          *  This method will create a producer_config and producer_votes object for 'producer'
          *
          *  @pre producer is not already registered
          *  @pre producer to register is an account
          *  @pre authority of producer to register
          *
          */
         static void on( const regproducer& reg ) {
            auto producer = reg.producer;
            require_auth( producer );

            producer_votes_index_type votes( SystemAccount, SystemAccount );
            auto existing = votes.find( producer );
            eosio_assert( existing == votes.end(), "producer already registered" );

            votes.emplace( producer, [&]( auto& pv ){
               pv.owner       = producer;
               pv.total_votes = 0;
            });

            producer_config_index_type proconfig( SystemAccount, SystemAccount );
            proconfig.emplace( producer, [&]( auto& pc ) {
               pc.owner      = producer;
               pc.packed_key = reg.producer_key;
            });
         }

         ACTION( SystemAccount, stakevote ) {
            account_name      voter;
            system_token_type amount;

            EOSLIB_SERIALIZE( stakevote, (voter)(amount) )
         };

         static void on( const stakevote& sv ) {
            print( "on stake vote\n" );
            eosio_assert( sv.amount.quantity > 0, "must stake some tokens" );
            require_auth( sv.voter );

            account_votes_index_type avotes( SystemAccount, SystemAccount );

            auto acv = avotes.find( sv.voter );
            if( acv == avotes.end() ) {
               acv = avotes.emplace( sv.voter, [&]( auto& av ) {
                 av.owner = sv.voter;
                 av.last_update = now();
                 av.proxy = 0;
               });
            }

            uint128_t old_weight = acv->staked.quantity;
            uint128_t new_weight = old_weight + sv.amount.quantity;

            producer_votes_index_type votes( SystemAccount, SystemAccount );

            for( auto p : acv->producers ) {
               votes.modify( votes.get( p ), 0, [&]( auto& v ) {
                  v.total_votes -= old_weight;
                  v.total_votes += new_weight;
               });
            }

            avotes.modify( acv, 0, [&]( auto av ) {
               av.last_update = now();
               av.staked += sv.amount;
            });

            currency::inline_transfer( sv.voter, SystemAccount, sv.amount, "stake for voting" );
         }

         ACTION( SystemAccount, voteproducer ) {
            account_name                voter;
            account_name                proxy;
            std::vector<account_name>   producers;

            EOSLIB_SERIALIZE( voteproducer, (voter)(proxy)(producers) )
         };

         /**
          *  @pre vp.producers must be sorted from lowest to highest
          *  @pre if proxy is set then no producers can be voted for
          *  @pre every listed producer or proxy must have been previously registered
          *  @pre vp.voter must authorize this action
          *  @pre voter must have previously staked some EOS for voting
          */
         static void on( const voteproducer& vp ) {
            eosio_assert( std::is_sorted( vp.producers.begin(), vp.producers.end() ), "producer votes must be sorted" );
            eosio_assert( vp.producers.size() <= 30, "attempt to vote for too many producers" );
            if( vp.proxy != 0 ) eosio_assert( vp.producers.size() == 0, "cannot vote for producers and proxy at same time" );

            require_auth( vp.voter );

            account_votes_index_type avotes( SystemAccount, SystemAccount );
            const auto& existing = avotes.get( vp.voter );

            std::map<account_name, pair<uint128_t, uint128_t> > producer_vote_changes;

            uint128_t old_weight = existing.staked.quantity; /// old time
            uint128_t new_weight = old_weight; /// TODO: update for current weight

            for( const auto& p : existing.producers )
               producer_vote_changes[p].first = old_weight;
            for( const auto& p : vp.producers )
               producer_vote_changes[p].second = new_weight;

            producer_votes_index_type votes( SystemAccount, SystemAccount );
            for( const auto& delta : producer_vote_changes ) {
               if( delta.second.first != delta.second.second ) {
                  const auto& provote = votes.get( delta.first );
                  votes.modify( provote, 0, [&]( auto& pv ){
                     pv.total_votes -= delta.second.first;
                     pv.total_votes += delta.second.second;
                  });
               }
            }

            avotes.modify( existing, 0, [&]( auto& av ) {
              av.proxy = vp.proxy;
              av.last_update = now();
              av.producers = vp.producers;
            });
         }

         static void on( const regproxy& reg ) {
            require_auth( reg.proxy_to_register );

         }

         static key256 to_key256(const checksum256& key) {
            std::array<uint8_t, 32> raw;
            std::copy( std::begin( key.hash ), std::end( key.hash ), std::begin( raw ) );
            return key256(raw);
         }

      /**
       * @pre setgen not previously applied
       * @param gen the map of pubkey to balance of genesis
       */
         static void on( const setgen& gen ) {
            require_auth( system_account );
            genesis_balance_index_type genesis_balances( system_account, system_account );
            eosio_assert( genesis_balances.begin() == genesis_balances.end(), "setgen already applied." );

            for( const auto& gb : gen.genesis_balances ) {
               key256 key = to_key256( gb.first );
               genesis_balances.emplace( system_account, [&key, &gb]( auto& v ) {
                  v.pubkey_hash = key;
                  v.balance = gb.second;
               });
            }
         }

         /**
          * @pre in transaction with an associated keyproof context free action
          * @param cg action to claim genesis balance for given account
          */
         static void on( const claimgen& cg ) {
            // verify associated keyproof context free action contains same pubkey_hash
            eosio::action cfa = eosio::get_action( 0, 0 );
            keyproof kp = cfa.data_as<keyproof>();
            eosio_assert( kp.pubkey_hash == cg.pubkey_hash, "claimgen pubkey_hash not equal to keyproof pubkey_hash" );

            genesis_balance_index_type genesis_balances( system_account, system_account );
            auto pubkeyidx = genesis_balances.template get_index<N(genpubkey)>();
            auto gbitr = pubkeyidx.find( to_key256(cg.pubkey_hash) );

            eosio_assert( gbitr != pubkeyidx.end(), "unable to find genesis pubkey_hash" );

            currency::inline_transfer( system_account, cg.name, gbitr->balance, "claimed genesis balance" );

            // remove genesis balance from claimable table
            pubkeyidx.erase( gbitr );
         }

         /**
          * Context free action.
          * Verifies associated context free data signature matches provided hashed public key.
          * @param kp the context free action data
          */
         static void on( const keyproof& kp ) {
            signature sig;
            int size = get_context_free_data( kp.cfd_idx, reinterpret_cast<char*>(&sig), sizeof(sig) );
            eosio_assert( size == sizeof(sig), "keyproof associated context free signature data wrong size" );

            public_key pk;
            recover_key( &kp.digest, reinterpret_cast<const char*>(&sig), sizeof(sig), reinterpret_cast<char*>(&pk), sizeof(pk) );

            assert_sha256(reinterpret_cast<char*>(&pk), sizeof(pk), &kp.pubkey_hash );
         }

         static void on( const nonce& ) {
         }

         static void on( const finishundel& ) {
         }

         static void apply( account_name code, action_name act ) {

            if( !eosio::dispatch<contract,
                                 regproducer, regproxy,
                                 delegatebw, undelegatebw,
                                 finishundel, voteproducer, stakevote,
                                 setgen, claimgen, keyproof,
                                 nonce>( code, act) ) {
               if ( !eosio::dispatch<currency, typename currency::transfer, typename currency::issue>( code, act ) ) {
                  eosio::print("Unexpected action: ", eosio::name(act), "\n");
                  eosio_assert( false, "received unexpected action");
               }
            }

         } /// apply
   };

} /// eosiosystem
