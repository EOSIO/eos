#include <eosio/chain/controller.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/chain/authority.hpp>
#include <eosio/chain/global_property_object.hpp>

#include <boost/program_options.hpp>

using namespace eosio;
using namespace eosio::chain;
using namespace std;

namespace bpo = boost::program_options;
using bpo::options_description;
using bpo::variables_map;

void generate_block_with_regproducer_trxs( controller& chain, const private_key_type& priv_key, const public_key_type& pub_key ) {
   ilog("Adding block with regproducer trx for each producer");
   auto abi = chain.get_account(N(eosio)).get_abi();
   chain::abi_serializer abis(abi, fc::microseconds::maximum());
   string action_type_name = abis.get_action_type(N(regproducer));
   FC_ASSERT( action_type_name != string(), "regproducer action is not found in the chain");

   auto next_time = fc::time_point::now();

   auto last_produced_block_num = chain.head_block_num();
   auto producer_in_charge = chain.head_block_state()->get_scheduled_producer(next_time).producer_name;
   auto& producer_to_last_produced = chain.head_block_state()->producer_to_last_produced;
   auto itr =  producer_to_last_produced.find(producer_in_charge);
   if ( itr != producer_to_last_produced.end() ) last_produced_block_num = itr->second;

   chain.start_block(next_time, chain.head_block_num() - last_produced_block_num, {});
   
   for ( auto& prod: chain.head_block_state()->active_schedule.producers ) {
      action act;
      act.account = N(eosio);
      act.name = N(regproducer);
      act.authorization = {{prod.producer_name, config::active_name}};
      act.data = abis.variant_to_binary( action_type_name,  
                                          mutable_variant_object()
                                             ("producer",      prod.producer_name)
                                             ("producer_key",  string(pub_key))
                                             ("url",           "")
                                             ("location",      0), 
                                          fc::microseconds::maximum() );

      signed_transaction trx;
      trx.actions.emplace_back(act);
      trx.expiration = next_time + fc::seconds(300);
      trx.set_reference_block(chain.head_block_id());
      trx.max_net_usage_words = 0; 
      trx.max_cpu_usage_ms = 0;
      trx.delay_sec = 0;
      trx.sign(priv_key, chain.get_chain_id());

      auto mtrx = std::make_shared<transaction_metadata>(trx);
      transaction_metadata::start_recover_keys(mtrx, chain.get_thread_pool(), chain.get_chain_id(), fc::microseconds::maximum());
      ilog("Call regproducer for ${prod}",("prod", prod.producer_name));
      auto result = chain.push_transaction(mtrx, fc::time_point::maximum());
      if( result->except_ptr ) std::rethrow_exception(result->except_ptr);
      if( result->except )  throw *result->except;
   }

   chain.finalize_block( [&]( const digest_type& d ) {
      return priv_key.sign(d);
   } );

   chain.commit_block();
}

void start_chain_from_snapshot( controller& chain, const fc::path& snapshot_input_path ) {
   ilog("Starting temporary chain from the snapshot ${path}", ("path", snapshot_input_path.string()));
   auto infile = std::ifstream(snapshot_input_path.string(), (std::ios::in | std::ios::binary));
   auto reader = std::make_shared<istream_snapshot_reader>(infile);
   reader->validate();
   chain.startup( []() { return false; }, reader );
   infile.close();
}

void create_snapshot( controller& chain, const fc::path& snapshot_output_path ) {
   ilog("Creating snapshot. Snapshot is written at ${path}", ("path", snapshot_output_path.string()));
   auto snap_out = std::ofstream(snapshot_output_path.string(), (std::ios::out | std::ios::binary));
   auto writer = std::make_shared<ostream_snapshot_writer>(snap_out);
   chain.write_snapshot(writer);
   writer->finalize();
   snap_out.flush();
   snap_out.close();
}

int main(int argc, char** argv) {
   int return_code = EXIT_SUCCESS;

   // Define the temp blocks and states directory, and ensure they are empty
   fc::temp_directory tempdir;
   auto blocks_dir = tempdir.path() / string("eosio-snapshot-modifier-").append(config::default_blocks_dir_name);
   auto state_dir = tempdir.path() / string("eosio-snapshot-modifier-").append(config::default_state_dir_name);
   fc::remove_all(blocks_dir);
   fc::remove_all(state_dir);

    try {
      fc::path snapshot_input_path;
      fc::path snapshot_output_path;
      public_key_type pub_key;
      private_key_type priv_key;
      uint64_t chain_state_db_size;
      bool should_generate_block;

      // Parse command line arguments
      variables_map vmap;
      options_description cli("EOSIO Snapshot Modifier Command Line Options");
      cli.add_options()
         ("input,i",bpo::value<string>(),"Path to the snapshot to be modified (Required)")
         ("public-key",bpo::value<string>(),"Public key which will be used to replace the producers' owner, active, and signing key (Required)")
         ("output-dir,o",bpo::value<string>()->default_value("./"), "Path to the directory where the modified snapshot will be written")
         ("generate-block,g", "Generate block with regproducer transactions for each producer. This must be used in conjuction with --private-key")
         ("private-key", bpo::value<string>(), "Private key used to authorize regproducer transaction and sign the block. This is only useful when used in conjuction with --generate-block/-g")
         ("chain-state-db-size-mb", bpo::value<uint64_t>()->default_value(1024), "Maximum size (in MiB) of the temporary chain state database. Must be big enough to accomodate the snapshot" )
         ("help,h","Print the list of commands available");
      bpo::store(bpo::parse_command_line(argc, argv, cli), vmap);
      bpo::notify(vmap);

      if (vmap.count("help") > 0) {
         cli.print(cerr);
         return EXIT_SUCCESS;
      }

      // Assertion on required parameterrs
      FC_ASSERT(vmap.count("input") > 0, "Missing path to the snapshot to be modified");
      FC_ASSERT(vmap.count("public-key") > 0, "Missing public key that will be used to replace the producers' owner, active, and signing key");

      // Extract input path 
      snapshot_input_path = fc::path(vmap["input"].as<string>());
      FC_ASSERT(fc::exists(snapshot_input_path), "Snapshot doesn't exist");

      // Extract output dir path, create the directories if it doesn't exist
      fc::path snapshot_output_dir_path(vmap["output-dir"].as<string>());
      if ( fc::exists(snapshot_output_dir_path) ) {
         FC_ASSERT(fc::is_directory(snapshot_output_dir_path), "Invalid output directory path");
      } else {
         fc::create_directories(snapshot_output_dir_path);
      }
      snapshot_output_path = snapshot_output_dir_path / string("modified-").append(snapshot_input_path.filename().string());

      // Extract public key
      try {
         pub_key = public_key_type(vmap["public-key"].as<string>());
      }  EOS_RETHROW_EXCEPTIONS(public_key_type_exception, "Invalid public key")

      // Extract private key, only if --generate block/-g is specified
      should_generate_block = vmap.count("generate-block") > 0;
      if (should_generate_block) {
         FC_ASSERT(vmap.count("private-key") > 0, "--private-key must always be used when --generate-block/-g is used");
          try {
            priv_key = private_key_type(vmap["private-key"].as<string>());
         }  EOS_RETHROW_EXCEPTIONS(private_key_type_exception, "Invalid private key")
         FC_ASSERT(priv_key.get_public_key() == pub_key, "Private and public key mismatch");
      }
      
      // Extract chain state size
      chain_state_db_size = vmap["chain-state-db-size-mb"].as<uint64_t>() * 1024 * 1024;

      controller::config cfg;
      cfg.blocks_dir = blocks_dir;
      cfg.state_dir = state_dir;
      cfg.state_size = chain_state_db_size;
      cfg.state_guard_size = 0;

      // Create a new chain from the snapshot
      controller chain(cfg, {});
      chain.add_indices();
      start_chain_from_snapshot(chain, snapshot_input_path);

      // Replace producer keys with the new key
      chain.replace_producer_keys(pub_key);

      // Add a block with regproducer called for each producer
      if (should_generate_block) generate_block_with_regproducer_trxs(chain, priv_key, pub_key);
   
      // Ensure proposed and pending producer schedule is empty (in case it is created by the new block)
      chain.clear_proposed_and_pending_producer_schedule();

      // Create the snapshot
      create_snapshot(chain, snapshot_output_path);

   }  catch( const fc::exception& e ) {
      elog("${e}", ("e", e.to_detail_string()));
      return_code = EXIT_FAILURE;
   } catch( const std::exception& e ) {
      elog("Caught Exception: ${e}", ("e",e.what()));
      return_code = EXIT_FAILURE;
   } catch( ... ) {
      elog("Unknown exception");
      return_code = EXIT_FAILURE;
   } 

   // Clean the temporary blocks and states dir
   ilog("Cleaning temporary blocks and states dir");
   fc::remove_all(blocks_dir);
   fc::remove_all(state_dir);

   return return_code;
}
