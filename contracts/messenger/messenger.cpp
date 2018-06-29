#include <eosiolib/eosio.hpp>
#include <eosiolib/transaction.hpp>
#include <eosiolib/crypto.h>
#include <string>

using eosio::indexed_by;
using eosio::const_mem_fun;
using std::string;


class messenger : public eosio::contract {
public:
   explicit messenger( action_name self )
         : contract( self ) {
   }

   //@abi action
   void sendmsg( account_name from, account_name to, uint64_t msg_id, const string& msg_str ) {
      eosio::print("sendmsg from ", eosio::name{from});
      // if not authorized then this action is aborted and transaction is rolled back
      // any modifications by other actions in same transaction are undone
      require_auth( from ); // make sure authorized by account

       // Add the specified account to set of accounts to be notified
      require_recipient( to );

      // calculate sha256 from msg_str
      checksum256 cs{};
      sha256( const_cast<char*>(msg_str.c_str()), msg_str.size(), &cs );

      // print for debugging
      printhex(&cs, sizeof(cs));

      // message_index is typedef of our multi_index over table address
      // message table is auto "created" if needed
      message_index messages( _self, _self ); // code, scope

      // verify does not already exist
      // multi_index find on primary index which in our case is account
      auto itr = messages.find( msg_id );
      if( itr != messages.end() ) {
         std::string err = "Message id already exists: " + std::to_string(msg_id);
         eosio_assert( false, err.c_str() );
      }

      // add to table, first argument is account to bill for storage
      // each entry will be billed to the associated account
      // we could have instead chosen to bill _self for all the storage
      messages.emplace( from /*payer*/, [&]( auto& msg ) {
         msg.msg_id = msg_id;
         msg.from = from;
         msg.to = to;
         msg.msg_sha = cs;
      } );

      // send msgnotify to 'to' account
      eosio::action( eosio::permission_level{from, N( active )},
                     to, N( msgnotify ), std::make_tuple( from, msg_id, cs) ).send();
   }

   //@abi action
   void sendsha( account_name from, account_name to, uint64_t msg_id, const checksum256& msg_sha ) {
      // if not authorized then this action is aborted and transaction is rolled back
      // any modifications by other actions in same transaction are undone
      require_auth( from ); // make sure authorized by account

      // Add the specified account to set of accounts to be notified
      require_recipient( to );

      // message_index is typedef of our multi_index over table address
      // message table is auto "created" if needed
      message_index messages( _self, _self ); // code, scope

      // verify does not already exist
      // multi_index find on primary index which in our case is account
      auto itr = messages.find( msg_id );
      if( itr != messages.end() ) {
         std::string err = "Message id already exists: " + std::to_string(msg_id);
         eosio_assert( false, err.c_str() );
      }

      // add to table, first argument is account to bill for storage
      // each entry will be billed to the associated account
      // we could have instead chosen to bill _self for all the storage
      messages.emplace( from /*payer*/, [&]( auto& msg ) {
         msg.msg_id = msg_id;
         msg.from = from;
         msg.to = to;
         msg.msg_sha = msg_sha;
      } );

      // send msgnotify to 'to' account
      eosio::action( eosio::permission_level{from, N( active )},
                     to, N( msgnotify ), std::make_tuple( from, msg_id, msg_sha) ).send();
   }

   //@abi action
   void removemsg( account_name to, uint64_t msg_id ) {
      require_auth( _self ); // make sure authorized by account

      message_index messages( _self, _self ); // code, scope

      // verify already exist
      auto itr = messages.find( msg_id );
      if( itr == messages.end() ) {
         std::string err = "Message does not exist: " + std::to_string(msg_id);
         eosio_assert( false, err.c_str() );
      }

      // verify correct to
      if( itr->to != to ) {
         std::string err = "Message with id " + std::to_string(msg_id) + " is to: " +
               eosio::name{itr->to}.to_string() + " not: " + eosio::name{to}.to_string();
         eosio_assert( false, err.c_str() );
      }

      messages.erase( itr );
   }

   //@abi action
   void removesha( const checksum256& msg_sha ) {
      require_auth( _self ); // make sure authorized by account

      message_index messages( _self, _self ); // code, scope

      auto index = messages.get_index<N( msgsha )>();
      auto itr = index.find( message::to_key( msg_sha ));
      if( itr == index.end() ) { // verify already exist
         printhex(&msg_sha, sizeof(msg_sha));
         eosio_assert( false, "Unable to find msg sha" );
      }

      index.erase( itr );
   }

   //@abi action
   void printmsg( account_name other ) {
      message_index messages( other, other ); // code, scope

      for (auto& msg : messages) {
         eosio::print("from: ", eosio::name{msg.from}, ", to: ", eosio::name{msg.to}, "\n");
      }
   }

   //@abi action
   void addgroup( account_name group_name, const std::vector<account_name>& accounts ) {
      require_auth( _self ); // make sure authorized by account

      group_index groups( _self, _self ); // code, scope

      auto itr = groups.find( group_name );
      if( itr != groups.end() ) {
         // found, so update
         groups.modify( itr, 0 /*payer*/, [&]( auto& group ) {
            group.accounts = accounts;
         });
      } else {
         groups.emplace( _self /*payer*/, [&]( auto& group ) {
            group.group_name = group_name;
            group.accounts = accounts;
         } );
      }
   }

   //@abi action
   void removegroup( account_name group_name ) {
      require_auth( _self ); // make sure authorized by account

      group_index groups( _self, _self ); // code, scope

      auto itr = groups.find( group_name );
      if( itr == groups.end() ) {
         std::string err = "group does not exist: " + eosio::name{group_name}.to_string();
         eosio_assert( false, err.c_str() );
      }

      groups.erase( itr );
   }

   //@abi action
   void sendgroup( account_name from, account_name group_name, uint64_t msg_id, const string& msg_str ) {
      require_auth( from );

      group_index groups( _self, _self ); // code, scope

      // calculate sha256 from msg_str
      checksum256 cs{};
      sha256( const_cast<char*>(msg_str.c_str()), msg_str.size(), &cs );

      // print for debugging
      printhex(&cs, sizeof(cs));

      // message_index is typedef of our multi_index over table address
      // message table is auto "created" if needed
      message_index messages( _self, _self ); // code, scope

      // verify does not already exist
      // multi_index find on primary index which in our case is account
      if( messages.find( msg_id ) != messages.end() ) {
         std::string err = "Message id already exists: " + std::to_string(msg_id);
         eosio_assert( false, err.c_str() );
      }

      // add to table, first argument is account to bill for storage
      // each entry will be billed to the associated account
      // we could have instead chosen to bill _self for all the storage
      messages.emplace( from /*payer*/, [&]( auto& msg ) {
         msg.msg_id = msg_id;
         msg.from = from;
         msg.to = group_name;
         msg.msg_sha = cs;
      } );

      auto itr = groups.find( group_name );
      if( itr != groups.end() ) {
         // found, so send
         for (auto& a : itr->accounts ) {

            // send msgnotify to 'to' account
            eosio::action( eosio::permission_level{from, N( active )},
                           a, N( msgnotify ), std::make_tuple( from, msg_id, cs) ).send();
         }
      } else {
         std::string err = "group does not exist: " + eosio::name{group_name}.to_string();
         eosio_assert( false, err.c_str() );
      }
   }

   //@abi action
   void addblacklist( account_name n) {
      require_auth( _self );

      blacklist_index blacklist( _self, _self );

      auto itr = blacklist.find( n );
      if( itr != blacklist.end() ) {
         // found, no need to waste cpu/bandwidth so assert
         eosio_assert( false, "Already in blacklist" );
      } else {
         blacklist.emplace( _self /*payer*/, [&]( auto& b ) {
            b.account = n;
         } );
      }
   }

   //@abi action
   void rmblacklist( account_name n ) {
      require_auth( _self );

      blacklist_index blacklist( _self, _self );

      auto itr = blacklist.find( n );
      if( itr != blacklist.end() ) {
         blacklist.erase( itr );
      } else {
         eosio_assert( false, "Not in blacklist" );
      }
   }

   //@abi action
   void spam( account_name from, account_name to, uint64_t msg_id, const string& msg_str, uint32_t delay_sec ) {
      require_auth( from );

      if( msg_id > 10 )
         return;

      {
         eosio::transaction out;
         out.actions.emplace_back( eosio::permission_level{from, N( active )}, to, N( sendmsg ),
                                   std::make_tuple( from, to, ++msg_id, msg_str ));
         out.delay_sec = delay_sec;
         out.send( msg_id, _self );
      }
      {
         eosio::transaction out;
         out.actions.emplace_back( eosio::permission_level{from, N( active )}, to, N( spam ),
                                   std::make_tuple( from, to, ++msg_id, msg_str, delay_sec ));
         out.delay_sec = delay_sec;
         out.send( msg_id, _self );
      }
   }

   //@abi action
   void msgnotify( account_name from, uint64_t /*msg_id*/, const checksum256& /*msg_sha*/ ) {
      eosio::print("message from ", eosio::name{from});
      blacklist_index blacklist( _self, _self );

      // if on blacklist assert
      auto itr = blacklist.find( from );
      if( itr != blacklist.end() ) {
         eosio_assert(false, "Account on blacklist");
      }
   }

private:

   //@abi table message i64
   struct message {
      uint64_t msg_id;      // unique identifier for message
      account_name from;    // account message sent from
      account_name to;      // account message sent to
      checksum256 msg_sha;  // sha256 of message string

      uint64_t primary_key() const { return msg_id; }

      eosio::key256 by_sha() const { return to_key( msg_sha ); }

      static eosio::key256 to_key( const checksum256& msg_sha ) {
         const uint64_t* ui64 = reinterpret_cast<const uint64_t*>(&msg_sha);
         return eosio::key256::make_from_word_sequence<uint64_t>( ui64[0], ui64[1], ui64[2], ui64[3] );
      }

   };

   typedef eosio::multi_index<N( message ), message,
         indexed_by<N( msgsha ), const_mem_fun<message, eosio::key256, &message::by_sha> >
   > message_index;


   //@abi table group i64
   struct group {
      account_name group_name;
      std::vector<account_name> accounts; /// The accounts that are member of this group

      uint64_t primary_key() const { return group_name; }
   };

   typedef eosio::multi_index<N( group ), group> group_index;

   //@abi table blacklist i64
   struct blacklist {
      account_name account;

      uint64_t primary_key() const { return account; }
   };

   typedef eosio::multi_index<N( blacklist ), blacklist> blacklist_index;

};

EOSIO_ABI( messenger, (sendmsg)( sendsha )( removemsg )( removesha )( printmsg )
                      ( addgroup )( removegroup )( sendgroup )
                      ( spam )
                      ( addblacklist )( rmblacklist )
                      ( msgnotify  ))
