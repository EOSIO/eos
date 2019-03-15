#pragma once

#include <eosiolib/singleton.hpp>
#include <eosiolib/multi_index.hpp>

namespace identity {

   typedef uint64_t identity_name;
   typedef uint64_t property_name;
   typedef uint64_t property_type_name;

   struct certvalue {
      property_name     property; ///< name of property, base32 encoded i64
      std::string       type; ///< defines type serialized in data
      std::vector<char> data; ///<
      std::string       memo; ///< meta data documenting basis of certification
      uint8_t           confidence = 1; ///< used to define liability for lies,
      /// 0 to delete

      EOSLIB_SERIALIZE( certvalue, (property)(type)(data)(memo)(confidence) )
   };

   struct certrow {
      uint64_t            id;
      property_name       property;
      uint64_t            trusted;
      account_name        certifier;
      uint8_t             confidence = 0;
      std::string         type;
      std::vector<char>   data;
      uint64_t primary_key() const { return id; }

      EOSLIB_SERIALIZE( certrow , (id)(property)(trusted)(certifier)(confidence)(type)(data) )
   };

   struct identrow {
      uint64_t     identity;
      account_name creator;

      uint64_t primary_key() const { return identity; }

      EOSLIB_SERIALIZE( identrow , (identity)(creator) )
   };

   struct trustrow {
      account_name account;

      uint64_t primary_key() const { return account; }

      EOSLIB_SERIALIZE( trustrow, (account) )
   };

   typedef eosio::multi_index<N(certs), certrow,
       eosio::indexed_by<N(bytuple), eosio::composite_key<certrow,
           eosio::member<certrow, property_name, &certrow::property>,
           eosio::member<certrow, uint64_t, &certrow::trusted>,
           eosio::member<certrow, account_name, &certrow::certifier>>>
       > certs_table;
   typedef eosio::multi_index<N(idents), identrow> idents_table;
   typedef eosio::singleton<N(account), identity_name>  accounts_table;
   typedef eosio::multi_index<N(trust), trustrow> trust_table;

   class identity_base {
      public:
         identity_base( account_name acnt) : _self( acnt ) {}

         bool is_trusted_by( account_name trusted, account_name by );

         bool is_trusted( account_name acnt );

      protected:
         account_name _self;
   };

}

