#include <eosiolib/eosio.hpp>
#include <eosiolib/crypto.h>
#include <string>

class message : public eosio::contract {
public:

   explicit message( account_name self ) : contract( self ) {}

   ///@abi action
   void sendmsg( account_name from, account_name to, uint64_t msg_id, const std::string& msg_str ) {
      require_auth( from );

      mess_index messages(_self, _self);

      auto itr = messages.find(msg_id);
      if (itr != messages.end()) {
         std::string err = "msg_id already exists: " + std::to_string(msg_id);
         eosio_assert(false, err.c_str());
//         eosio_assert(false, "msg_id already exists");
      }

      checksum256 cs{};
      sha256( const_cast<char*>(msg_str.c_str()), msg_str.size(), &cs);

      printhex(&cs, sizeof(checksum256));

      messages.emplace( _self /*payer*/, [&](auto& msg){
         msg.msg_id = msg_id;
         msg.from = from;
         msg.to = to;
         msg.msg_sha = cs;
      });
   }

   ///@abi action
   void sendsha( account_name from, account_name to, uint64_t msg_id, const checksum256& msg_sha ) {
      require_auth( from );

      mess_index messages(_self, _self);

      auto itr = messages.find(msg_id);
      if (itr != messages.end()) {
         std::string err = "msg_id already exists: " + std::to_string(msg_id);
         eosio_assert(false, err.c_str());
//         eosio_assert(false, "msg_id already exists");
      }

      messages.emplace( _self /*payer*/, [&](auto& msg){
         msg.msg_id = msg_id;
         msg.from = from;
         msg.to = to;
         msg.msg_sha = msg_sha;
      });
   }

   void removemsg( uint64_t msg_id ) {}

   void removesha( const checksum256& msg_sha ) {}

private:

   ///@abi table mess i64
   struct mess {
      uint64_t msg_id;     // some unique id
      account_name from;
      account_name to;
      checksum256 msg_sha; // sha256 of the message

      uint64_t primary_key() const { return msg_id; }
   };

   typedef eosio::multi_index<N(mess), mess> mess_index;


};

EOSIO_ABI( message, (sendmsg)(sendsha) )
