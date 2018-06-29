#include <eosiolib/eosio.hpp>
#include <eosiolib/crypto.h>
#include <boost/container/flat_map.hpp>
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
      }

      checksum256 cs{};
      sha256( const_cast<char*>(msg_str.c_str()), msg_str.size(), &cs);

      printhex(&cs, sizeof(checksum256));

      messages.emplace( from /*payer*/, [&](auto& msg){
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
      }

      messages.emplace( _self /*payer*/, [&](auto& msg){
         msg.msg_id = msg_id;
         msg.from = from;
         msg.to = to;
         msg.msg_sha = msg_sha;
      });
   }

   //@abi action
   void printmsg( account_name other ) {
      mess_index messages( other, other ); // code, scope

      boost::container::flat_map<account_name, account_name> map;

      for (auto& msg : messages) {
         map[msg.from] = msg.to;
         eosio::print("from: ", eosio::name{msg.from}, ", to: ", eosio::name{map[msg.from]}, "\n");
      }
      for (auto& m : map) {
         eosio::print("from: ", eosio::name{m.first}, ", to: ", eosio::name{m.second}, "\n");
      }
   }

   //@abi action
   void ppp(const std::vector<account_name>& accounts) {

      boost::container::flat_map<account_name, account_name> map;
      for (size_t i = 0; i < accounts.size(); i+=2) {
         map[accounts[i]] = accounts[i+1];
      }

      for (auto& m : map) {
         eosio::print("from: ", eosio::name{m.first}, ", to: ", eosio::name{m.second}, "\n");
      }
   }


   //@abi action
   void removemsg( uint64_t msg_id, const checksum256& msg_sha ) {
      require_auth(_self);

      mess_index messages(_self, _self);

      auto itr = messages.find(msg_id);
      if (itr == messages.end()) {
         std::string err = "msg_id does not exist: " + std::to_string(msg_id);
         eosio_assert(false, err.c_str());
      }

      messages.erase(itr);
   }

   //@abi action
   void removesha( const checksum256& msg_sha ) {
      require_auth(_self);

      mess_index messages(_self, _self);

      auto index = messages.get_index<N(msgsha)>();

      auto itr = index.find(mess::to_key(msg_sha));
      if (itr == index.end()) {
         eosio_assert( false, "msg_sha does not exist");
      }

      index.erase(itr);
   }

   //@abi action
   void other( account_name n) {
      // no require_auth because this is just accessing public table data

      mess_index messages(n, n);

      for (auto& msg : messages) {
         eosio::print("from: ", eosio::name{msg.from}, ", to: ", eosio::name{msg.to}, "\n");
      }
      
   }

private:

   ///@abi table mess i64
   struct mess {
      uint64_t msg_id;     // some unique id
      account_name from;
      account_name to;
      checksum256 msg_sha; // sha256 of the message

      uint64_t primary_key() const { return msg_id; }

      eosio::key256 by_sha() const { return to_key( msg_sha ); }

      static eosio::key256 to_key( const checksum256& sha ) {
         const uint64_t* ui64 = reinterpret_cast<const uint64_t*>(&sha);
         return eosio::key256::make_from_word_sequence<uint64_t>( ui64[0], ui64[1], ui64[2], ui64[3] );
      }

   };

   typedef eosio::multi_index<N(mess), mess,
                              eosio::indexed_by<N(msgsha), eosio::const_mem_fun<mess, eosio::key256, &mess::by_sha>>
   > mess_index;


};

EOSIO_ABI( message, (sendmsg)(sendsha)(printmsg)(ppp)(removesha)(removemsg)(other) )
