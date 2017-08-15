#pragma once
#include <appbase/application.hpp>
#include <eos/chain/chain_controller.hpp>
#include <eos/chain/key_value_object.hpp>
#include <eos/chain/account_object.hpp>

#include <eos/database_plugin/database_plugin.hpp>

namespace fc { class variant; }

namespace eos {
   using eos::chain::chain_controller;
   using std::unique_ptr;
   using namespace appbase;
   using chain::Name;
   using fc::optional;
   using chain::uint128_t;

namespace chain_apis {
struct empty{};

class read_only {
   const chain_controller& db;

public:
   read_only(const chain_controller& db)
      : db(db) {}

   using get_info_params = empty;

   struct get_info_results {
      uint32_t              head_block_num = 0;
      uint32_t              last_irreversible_block_num = 0;
      chain::block_id_type  head_block_id;
      fc::time_point_sec    head_block_time;
      types::AccountName    head_block_producer;
      string                recent_slots;
      double                participation_rate = 0;
   };
   get_info_results get_info(const get_info_params&) const;

   struct producer_info {
      Name                       name;
   };

   struct get_account_results {
      Name                       name;
      uint64_t                   eos_balance       = 0;
      uint64_t                   staked_balance    = 0;
      uint64_t                   unstaking_balance = 0;
      fc::time_point_sec         last_unstaking_time;
      optional<producer_info>    producer;
      optional<types::Abi>       abi;
   };
   struct get_account_params {
      Name name;
   };
   get_account_results get_account( const get_account_params& params )const;

   struct abi_json_to_bin_params {
      Name         code;
      Name         action;
      fc::variant  args;
   };
   struct abi_json_to_bin_result {
      vector<char>   binargs;
      vector<Name>   required_scope;
      vector<Name>   required_auth;
   };
      
   abi_json_to_bin_result abi_json_to_bin( const abi_json_to_bin_params& params )const;


   struct abi_bin_to_json_params {
      Name         code;
      Name         action;
      vector<char> binargs;
   };
   struct abi_bin_to_json_result {
      fc::variant    args;
      vector<Name>   required_scope;
      vector<Name>   required_auth;
   };
      
   abi_bin_to_json_result abi_bin_to_json( const abi_bin_to_json_params& params )const;



   struct get_block_params {
      string block_num_or_id;
   };

   struct get_block_results : public chain::signed_block {
      get_block_results( const chain::signed_block& b )
      :signed_block(b),
      id(b.id()),
      block_num(b.block_num()),
      refBlockPrefix( id._hash[1] )
      {}

      chain::block_id_type id;
      uint32_t             block_num = 0;
      uint32_t             refBlockPrefix = 0;
   };

   get_block_results get_block(const get_block_params& params) const;

   struct get_table_rows_i64_params {
      bool        json = false;
      Name        scope;
      Name        code;
      Name        table;
      uint64_t    lower_bound = 0;
      uint64_t    upper_bound = uint64_t(-1);
      uint32_t    limit = 10;
   };

   struct get_table_rows_i64_result {
      vector<fc::variant> rows; ///< one row per item, either encoded as hex String or JSON object 
      bool                more; ///< true if last element in data is not the end and sizeof data() < limit
   };

   get_table_rows_i64_result get_table_rows_i64( const get_table_rows_i64_params& params )const;

   struct get_table_rows_i128i128_primary_params {
      bool        json = false;
      Name        scope;
      Name        code;
      Name        table;
      uint128_t   lower_bound = 0;
      uint128_t   upper_bound = uint128_t(-1);
      uint32_t    limit = 10;
   };

   struct get_table_rows_i128i128_primary_result {
      vector<fc::variant> rows; ///< one row per item, either encoded as hex String or JSON object 
      bool                more; ///< true if last element in data is not the end and sizeof data() < limit
   };

   get_table_rows_i128i128_primary_result get_table_rows_i128i128_primary( const get_table_rows_i128i128_primary_params& params )const;   
};

class read_write {
   chain_controller& db;
   uint32_t skip_flags;
public:
   read_write(chain_controller& db, uint32_t skip_flags) : db(db), skip_flags(skip_flags) {}

   using push_block_params = chain::signed_block;
   using push_block_results = empty;
   push_block_results push_block(const push_block_params& params);

   using push_transaction_params = chain::SignedTransaction;
   struct push_transaction_results {
      chain::transaction_id_type  transaction_id;
      fc::variant                 processed;
   };
   push_transaction_results push_transaction(const push_transaction_params& params);
};
} // namespace chain_apis

class chain_plugin : public plugin<chain_plugin> {
public:
   APPBASE_PLUGIN_REQUIRES((database_plugin))

   chain_plugin();
   virtual ~chain_plugin();

   virtual void set_program_options(options_description& cli, options_description& cfg) override;

   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

   chain_apis::read_only get_read_only_api() const { return chain_apis::read_only(chain()); }
   chain_apis::read_write get_read_write_api();

   bool accept_block(const chain::signed_block& block, bool currently_syncing);
   void accept_transaction(const chain::SignedTransaction& trx);

   bool block_is_on_preferred_chain(const chain::block_id_type& block_id);

   // Only call this after plugin_startup()!
   chain_controller& chain();
   // Only call this after plugin_startup()!
   const chain_controller& chain() const;

  void get_chain_id (chain::chain_id_type &cid) const;

private:
   unique_ptr<class chain_plugin_impl> my;
};

}

FC_REFLECT(eos::chain_apis::empty, )
FC_REFLECT(eos::chain_apis::read_only::get_info_results,
           (head_block_num)(last_irreversible_block_num)(head_block_id)(head_block_time)(head_block_producer)
           (recent_slots)(participation_rate))
FC_REFLECT(eos::chain_apis::read_only::get_block_params, (block_num_or_id))

FC_REFLECT_DERIVED( eos::chain_apis::read_only::get_block_results, (eos::chain::signed_block), (id)(block_num)(refBlockPrefix) );
FC_REFLECT( eos::chain_apis::read_write::push_transaction_results, (transaction_id)(processed) )
FC_REFLECT( eos::chain_apis::read_only::get_table_rows_i64_params,
           (json)(scope)(code)(table)(lower_bound)(upper_bound)(limit) );
FC_REFLECT( eos::chain_apis::read_only::get_table_rows_i64_result,
           (rows)(more) );
FC_REFLECT( eos::chain_apis::read_only::get_table_rows_i128i128_primary_params,
           (json)(scope)(code)(table)(lower_bound)(upper_bound)(limit) );
FC_REFLECT( eos::chain_apis::read_only::get_table_rows_i128i128_primary_result,
           (rows)(more) );
FC_REFLECT( eos::chain_apis::read_only::get_account_results, (name)(eos_balance)(staked_balance)(unstaking_balance)(last_unstaking_time)(producer)(abi) )
FC_REFLECT( eos::chain_apis::read_only::get_account_params, (name) )
FC_REFLECT( eos::chain_apis::read_only::producer_info, (name) )
FC_REFLECT( eos::chain_apis::read_only::abi_json_to_bin_params, (code)(action)(args) )
FC_REFLECT( eos::chain_apis::read_only::abi_json_to_bin_result, (binargs)(required_scope)(required_auth) )
FC_REFLECT( eos::chain_apis::read_only::abi_bin_to_json_params, (code)(action)(binargs) )
FC_REFLECT( eos::chain_apis::read_only::abi_bin_to_json_result, (args)(required_scope)(required_auth) )
