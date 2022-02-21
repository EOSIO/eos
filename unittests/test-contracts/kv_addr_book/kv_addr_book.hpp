#include <eosio/eosio.hpp>

using namespace eosio;
using namespace std;

// this structure defines the data stored in the kv::table
struct person {
   eosio::name account_name;
   eosio::non_unique<eosio::name, std::string> first_name;
   eosio::non_unique<eosio::name, std::string> last_name;
   eosio::non_unique<eosio::name, std::string, std::string, std::string, std::string> street_city_state_cntry;
   eosio::non_unique<eosio::name, std::string> personal_id;
   std::pair<std::string, std::string> country_personal_id;

   eosio::name get_account_name() const {
      return account_name;
   }
   std::string get_first_name() const {
      // from the non_unique tuple we extract the value with key 1, the first name
      return std::get<1>(first_name);
   }
   std::string get_last_name() const {
      // from the non_unique tuple we extract the value with key 1, the last name
      return std::get<1>(last_name);
   }
   std::string get_street() const {
      // from the non_unique tuple we extract the value with key 1, the street
      return std::get<1>(street_city_state_cntry);
   }
   std::string get_city() const {
      // from the non_unique tuple we extract the value with key 2, the city
      return std::get<2>(street_city_state_cntry);
   }
   std::string get_state() const {
      // from the non_unique tuple we extract the value with key 3, the state
      return std::get<3>(street_city_state_cntry);
   }
   std::string get_country() const {
      // from the non_unique tuple we extract the value with key 4, the country
      return std::get<4>(street_city_state_cntry);
   }
   std::string get_personal_id() const {
      // from the non_unique tuple we extract the value with key 1, the personal id
      return std::get<1>(personal_id);
   }
};

// helper factory to easily build person objects
struct person_factory {
   static person get_person(
      eosio::name account_name,
      std::string first_name,
      std::string last_name,
      std::string street,
      std::string city,
      std::string state, 
      std::string country,
      std::string personal_id) {
         return person {
            .account_name = account_name,
            .first_name = {account_name, first_name},
            .last_name = {account_name, last_name},
            .street_city_state_cntry = {account_name, street, city, state, country},
            .personal_id = {account_name, personal_id},
            .country_personal_id = {country, personal_id}
         };
      }
};

class [[eosio::contract]] kv_addr_book : public eosio::contract {

   struct [[eosio::table]] address_table : eosio::kv::table<person, "kvaddrbook"_n> {
      // unique indexes definitions
      // 1. they are defined for just one property of the kv::table parameter type (person)
      // 2. unique indexes for multiple properties of the kv::table parameter type
      //    are defined with the help of a pair or a tuple; a pair if the index has 
      //    two properties or a tuple in case of more than two
      index<name> account_name_uidx {
         name{"accname"_n},
         &person::account_name };
      index<pair<std::string, std::string>> country_personal_id_uidx {
         name{"cntrypersid"_n},
         &person::country_personal_id };
      
      // non-unique indexes definitions
      // 1. non unique indexes need to be defined for at least two properties, 
      // 2. the first one needs to be a property that stores unique values, because 
      //    under the hood every index (non-unique or unique) is stored as an unique 
      //    index, and by providing as the first property one that has unique values
      //    it ensures the uniques of the values combined (including non-unique ones)
      // 3. the rest of the properties are the ones wanted to be indexed non-uniquely
      index<non_unique<eosio::name, std::string>> first_name_idx {
         name{"firstname"_n},
         &person::first_name};
      index<non_unique<eosio::name, std::string>> last_name_idx {
         name{"lastname"_n},
         &person::last_name};
      index<non_unique<eosio::name, std::string>> personal_id_idx {
         name{"persid"_n},
         &person::personal_id};
      // non-unique index defined using the KV_NAMED_INDEX macro
      // note: you can not name your index like you were able to do before (ending in `_idx`),
      // instead when using the macro you have to pass the name of the data member which 
      // is indexed and that will give the name of the index as well
      KV_NAMED_INDEX("address"_n, street_city_state_cntry)

      address_table(eosio::name contract_name) {
         init(contract_name,
            account_name_uidx,
            country_personal_id_uidx,
            first_name_idx,
            last_name_idx,
            personal_id_idx,
            street_city_state_cntry);
      }
   };

   public:
      using contract::contract;
      kv_addr_book(eosio::name receiver, eosio::name code, eosio::datastream<const char*> ds)
         : contract(receiver, code, ds) {}

      // retrieves a person based on primary key account_name
      [[eosio::action]]
      person get(eosio::name account_name);

      // retrieves a person based on unique index defined by country and personal_id
      [[eosio::action]]
      person getbycntrpid(std::string country, std::string personal_id);

      // retrieves list of persons with the same last name
      [[eosio::action]]
      std::vector<person> getbylastname(std::string last_name);

      // retrieves list of persons with the same address
      [[eosio::action]]
      std::vector<person> getbyaddress(
         std::string street, 
         std::string city, 
         std::string state, 
         std::string country);


      [[eosio::action]]
      void test();

      // creates if not exists, or updates if already exists, a person
      [[eosio::action]]
      void upsert(eosio::name account_name,
         std::string first_name,
         std::string last_name,
         std::string street,
         std::string city,
         std::string state, 
         std::string country,
         std::string personal_id);

      // deletes a person based on primary key account_name
      [[eosio::action]]
      void del(eosio::name account_name);

      // checks if a person exists in addressbook with a specific personal_id and country
      [[eosio::action]]
      bool checkpidcntr(std::string personal_id, std::string country);

      // iterates over the first iterations_count persons in the table 
      // and prints their first and last names
      [[eosio::action]]
      void iterate(int iterations_count);

      using get_action = eosio::action_wrapper<"get"_n, &kv_addr_book::get>;
      using get_by_cntry_pers_id_action = eosio::action_wrapper<"getbycntrpid"_n, &kv_addr_book::getbycntrpid>;
      using get_by_last_name_action = eosio::action_wrapper<"getbylastname"_n, &kv_addr_book::getbylastname>;
      using get_buy_address_action = eosio::action_wrapper<"getbyaddress"_n, &kv_addr_book::getbyaddress>;
      using upsert_action = eosio::action_wrapper<"upsert"_n, &kv_addr_book::upsert>;
      using del_action = eosio::action_wrapper<"del"_n, &kv_addr_book::del>;
      using is_pers_id_in_cntry_action = eosio::action_wrapper<"checkpidcntr"_n, &kv_addr_book::checkpidcntr>;
      using iterate_action = eosio::action_wrapper<"iterate"_n, &kv_addr_book::iterate>;

   private:
      void print_person(const person& person);
      address_table addresses{"kvaddrbook"_n};
};
