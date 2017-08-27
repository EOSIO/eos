#include <string>
#include <vector>
#include <boost/asio.hpp>
#include <iostream>
#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <fc/exception/exception.hpp>

#include <eos/chain/config.hpp>
#include <eos/chain_plugin/chain_plugin.hpp>
#include <eos/account_history_api_plugin/account_history_api_plugin.hpp>

#include <eos/utilities/key_conversion.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <boost/algorithm/string/split.hpp>

#include <Inline/BasicTypes.h>
#include <IR/Module.h>
#include <IR/Validate.h>
#include <WAST/WAST.h>
#include <WASM/WASM.h>
#include <Runtime/Runtime.h>

#include <fc/io/fstream.hpp>


#include <eos/py_plugin/py_plugin.hpp>
#include <fc/log/logger_config.hpp>
#include <boost/thread.hpp>

#include <python.h>


using namespace std;
using namespace eos;
using namespace eos::chain;
using namespace eos::utilities;


inline std::vector<Name> sort_names( std::vector<Name>&& names ) {
   std::sort( names.begin(), names.end() );
   auto itr = std::unique( names.begin(), names.end() );
   names.erase( itr, names.end() );
   return names;
}

//   chain_apis::read_write get_read_write_api();
extern "C" int get_info_(char *result,int length) {
    auto ro_api = app().get_plugin<chain_plugin>().get_read_only_api();
//    auto rw_api = app().get_plugin<chain_plugin>().get_read_write_api();
    chain_apis::read_only::get_info_params params;
    auto info = ro_api.get_info(params);
    
    auto json_str = fc::json::to_string(info);
    printf("json_str.size():%lu\n",json_str.size());
    if (json_str.size()>length){
      return -1;
    }
    strcpy(result,json_str.data());
//    ilog(result);
    return 0;
}

extern "C" int get_block_(int id,char *result,int length) {
    auto ro_api = app().get_plugin<chain_plugin>().get_read_only_api();
    char str_id[64];
    sprintf(str_id,"%d",id);
    chain_apis::read_only::get_block_params params = {str_id};
    auto info = ro_api.get_block(params);
    auto json_str = fc::json::to_string(info);
    if (json_str.size()>length){
      return -1;
    }
    strcpy(result,json_str.data());
    return 0;
}

extern "C" int get_account_(char* name,char *result,int length) {
    if (name == NULL || result == NULL){
        return -1;
    }
    auto ro_api = app().get_plugin<chain_plugin>().get_read_only_api();
    chain_apis::read_only::get_account_params params = {name};
    auto info = ro_api.get_account(params);
    auto json_str = fc::json::to_string(info);
    if (json_str.size()>length){
      return -1;
    }
    strcpy(result,json_str.data());
    return 0;
}


extern "C" void create_account_( char* creator_,char* newaccount_,char* owner_key_,char* active_key_ ) {
    auto rw_api = app().get_plugin<chain_plugin>().get_read_write_api();
    auto ro_api = app().get_plugin<chain_plugin>().get_read_only_api();

    Name creator(creator_);
    Name newaccount(newaccount_);
    Name eosaccnt(config::EosContractName);
    Name staked("staked");

    public_key_type owner_key(owner_key_);
    public_key_type active_key(active_key_);

    auto owner_auth   = eos::chain::Authority{1, {{owner_key, 1}}, {}};
    auto active_auth  = eos::chain::Authority{1, {{active_key, 1}}, {}};
    auto recovery_auth = eos::chain::Authority{1, {}, {{{creator, "active"}, 1}}};

    uint64_t deposit = 1;

    SignedTransaction trx;
    trx.scope = sort_names({creator,eosaccnt});
    transaction_helpers::emplace_message(trx, config::EosContractName, vector<types::AccountPermission>{{creator,"active"}}, "newaccount",
                       types::newaccount{creator, newaccount, owner_auth,
                                         active_auth, recovery_auth, deposit});

    chain_apis::read_only::get_info_params params;
    auto info = ro_api.get_info(params);
    trx.expiration = info.head_block_time + 100; //chain.head_block_time() + 100;
    transaction_helpers::set_reference_block(trx, info.head_block_id);
    boost::sort( trx.scope );
    try {
        std::cout << fc::json::to_pretty_string( rw_api.push_transaction(trx) ) << std::endl;
    }
    catch ( const fc::exception& e ) {
      elog((e.to_detail_string()));
    }
}

extern "C" int create_key_(char *pub_,int pub_length,char *priv_,int priv_length){
      if (pub_ == NULL || priv_ == NULL) {
        return -1;
      }

      auto priv = fc::ecc::private_key::generate();
      auto pub = public_key_type( priv.get_public_key() );

      string str_pub = string(pub);
      if (str_pub.size() > pub_length-1){
          return -1;
      }
      strcpy(pub_,str_pub.data());

      string str_priv = string(key_to_wif(priv.get_secret()));
      if (str_priv.size() > priv_length-1){
          return -1;
      }
      strcpy(priv_,str_priv.data());
      std::cout << "public: " << str_pub <<"\n";
      std::cout << "private: " << str_priv << std::endl;
      return 0;
}

extern "C" int get_transaction_(char *id,char* result,int length){
    if (id==NULL || result==NULL){
        return -1;
    }
    auto ro_api = app().get_plugin<account_history_plugin>().get_read_only_api();
    auto rw_api = app().get_plugin<account_history_plugin>().get_read_write_api();
    ilog(id);
    account_history_apis::read_only::get_transaction_params params = {chain::transaction_id_type(id)};
    ilog("before get_transaction.");
//    auto arg= fc::mutable_variant_object( "transaction_id", id);
    string str_ts = fc::json::to_pretty_string(ro_api.get_transaction(params));
    ilog(str_ts);
    if (str_ts.size()>length-1){
        return -1;
    }
    strcpy(result,str_ts.data());
    return 0;
}


namespace eos {

class py_plugin_impl {
   public:
};

py_plugin::py_plugin():my(new py_plugin_impl()){}
py_plugin::~py_plugin(){}

void py_plugin::set_program_options(options_description&, options_description& cfg) {
   cfg.add_options()
         ("option-name", bpo::value<string>()->default_value("default value"),
          "Option Description")
         ;
}

void py_plugin::plugin_initialize(const variables_map& options) {
   if(options.count("option-name")) {
      // Handle the option
   }
}

extern "C" void c_printf(const char *s);
extern "C" void PyInit_eosapi();

void py_thread() {
    Py_Initialize();
    PyRun_SimpleString("import readline");
    PyInit_eosapi();
    PyRun_SimpleString("import eosapi");
    ilog("++++++++++++++py_plugin::plugin_startup");
    c_printf("hello,world");
//    get_info();
    PyRun_InteractiveLoop(stdin, "<stdin>");
    Py_Finalize();
}

void py_plugin::plugin_startup() {  
    boost::thread t{py_thread};
}

void py_plugin::plugin_shutdown() {
    ilog("py_plugin::plugin_shutdown()");
    Py_Finalize();
}

}
