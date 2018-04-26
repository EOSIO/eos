/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/eosio.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/contract.hpp>
#include <eosiolib/print.hpp>
#include <string>

using eosio::indexed_by;
using eosio::const_mem_fun;
using std::string;

struct hi {
    account_name account;
};

class addressbook : public eosio::contract {
   public:
      explicit addressbook(action_name self)
              : contract(self) {
      }

      //@abi action
      void add(const account_name account,
               const string& first_name, const string& last_name,
               const string& street, const string& city, const string& state,
               uint32_t zip) {

          // if not authorized then this action is aborted and transaction is rolled back
          // any modifications by other actions are undone
          require_auth(account); // make sure authorized by account

          // address_index is typedef of our multi_index over table address
          // address table is auto "created" if needed
          address_index addresses(_self, _self); // code, scope

          // verify does not already exist
          // multi_index find on primary index which in our case is account
          auto itr = addresses.find(account);
          eosio_assert(itr == addresses.end(), "Address for account already exists");

          // add to table, first argument is account to bill for storage
          // each entry will be pilled to the associated account
          // we could have instead chosen to bill _self for all the storage
          addresses.emplace(account /*payer*/, [&](auto& address) {
              address.account_name = account;
              address.first_name = first_name;
              address.last_name = last_name;
              address.street = street;
              address.city = city;
              address.state = state;
              address.zip = zip;
              address.liked = 0;
          });

          eosio::action(
                  std::vector<eosio::permission_level>(1,{_self, N(active)}),
                  N(hello), N(hi), hi{account} ).send();
      }

      //@abi action
      void update(const account_name account,
                  const string& first_name, const string& last_name,
                  const string& street, const string& city, const string& state,
                  uint32_t zip) {

          require_auth(account); // make sure authorized by account

          address_index addresses(_self, _self); // code, scope

          // verify already exist
          auto itr = addresses.find(account);
          eosio_assert(itr != addresses.end(), "Address for account not found");

          addresses.modify( itr, account /*payer*/, [&]( auto& address ) {
              address.account_name = account;
              address.first_name = first_name;
              address.last_name = last_name;
              address.street = street;
              address.city = city;
              address.state = state;
              address.zip = zip;
          });

      }

      //@abi action
      void remove(const account_name account) {
          require_auth(account); // make sure authorized by account

          address_index addresses(_self, _self); // code, scope

          // verify already exist
          auto itr = addresses.find(account);
          eosio_assert(itr != addresses.end(), "Address for account not found");

          addresses.erase( itr );
      }

      //@abi action
      void like(const account_name account) {
          // do not require_auth since want to allow anyone to call

          address_index addresses(_self, _self); // code, scope

          // verify already exist
          auto itr = addresses.find(account);
          eosio_assert(itr != addresses.end(), "Address for account not found");

          addresses.modify( itr, 0 /*payer*/, [&]( auto& address ) {
              eosio::print("Liking: ",
                           address.first_name.c_str(), " ", address.last_name.c_str(), "\n");
              address.liked++;
          });
      }

      //@abi action
      void likezip( uint32_t zip ) {
          // do not require_auth since want to allow anyone to call

          address_index addresses(_self, _self); // code, scope

          auto zip_index = addresses.get_index<N(zip)>();
          auto itr = zip_index.lower_bound(zip);
          for (; itr != zip_index.end() && itr->zip == zip; ++itr) {
              zip_index.modify( itr, 0, [&]( auto &address ) {
                  eosio::print("Liking: ",
                               address.first_name.c_str(), " ", address.last_name.c_str(), " ");
                  address.liked++;
              });
          }
      }

   private:

      //@abi table address i64
      struct address {
          uint64_t account_name;
          string first_name;
          string last_name;
          string street;
          string city;
          string state;
          uint32_t zip = 0;
          uint64_t liked = 0;

          uint64_t primary_key() const { return account_name; }
          uint64_t by_zip() const { return zip; }

          EOSLIB_SERIALIZE( address, (account_name)(first_name)(last_name)(street)(city)(state)(zip)(liked) )
      };

      typedef eosio::multi_index< N(address), address,
         indexed_by< N(zip), const_mem_fun<address, uint64_t, &address::by_zip> >
      > address_index;

};

EOSIO_ABI( addressbook, (add)(update)(remove)(like)(likezip) )
