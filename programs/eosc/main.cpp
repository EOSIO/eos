#include <string>
#include <vector>
#include <boost/asio.hpp>
#include <iostream>
#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <fc/exception/exception.hpp>

#include <eos/chain/config.hpp>
#include <eos/chain_plugin/chain_plugin.hpp>
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

using namespace std;
using namespace eos;
using namespace eos::chain;
using namespace eos::utilities;

string program = "eosc";
string host = "localhost";
uint32_t port = 8888;

const string chain_func_base = "/v1/chain";
const string get_info_func = chain_func_base + "/get_info";
const string push_txn_func = chain_func_base + "/push_transaction";
const string json_to_bin_func = chain_func_base + "/abi_json_to_bin";
const string get_block_func = chain_func_base + "/get_block";
const string get_account_func = chain_func_base + "/get_account";

const string account_history_func_base = "/v1/account_history";
const string get_transaction_func = account_history_func_base + "/get_transaction";
const string get_transactions_func = account_history_func_base + "/get_transactions";

inline std::vector<Name> sort_names( std::vector<Name>&& names ) {
   std::sort( names.begin(), names.end() );
   auto itr = std::unique( names.begin(), names.end() );
   names.erase( itr, names.end() );
   return names;
}

vector<uint8_t> assemble_wast( const std::string& wast ) {
   IR::Module module;
   std::vector<WAST::Error> parseErrors;
   WAST::parseModule(wast.c_str(),wast.size(),module,parseErrors);
   if(parseErrors.size())
   {
      // Print any parse errors;
      std::cerr << "Error parsing WebAssembly text file:" << std::endl;
      for(auto& error : parseErrors)
      {
         std::cerr << ":" << error.locus.describe() << ": " << error.message.c_str() << std::endl;
         std::cerr << error.locus.sourceLine << std::endl;
         std::cerr << std::setw(error.locus.column(8)) << "^" << std::endl;
      }
      FC_ASSERT( !"error parsing wast" );
   }

   try
   {
      // Serialize the WebAssembly module.
      Serialization::ArrayOutputStream stream;
      WASM::serialize(stream,module);
      return stream.getBytes();
   }
   catch(Serialization::FatalSerializationException exception)
   {
      std::cerr << "Error serializing WebAssembly binary file:" << std::endl;
      std::cerr << exception.message << std::endl;
      throw;
   }
}



fc::variant call( const std::string& server, uint16_t port,
                  const std::string& path,
                  const fc::variant& postdata = fc::variant() );

template<typename T>
fc::variant call( const std::string& server, uint16_t port,
                  const std::string& path,
                  const T& v ) { return call( server, port, path, fc::variant(v) ); }

template<typename T>
fc::variant call( const std::string& path,
                  const T& v ) { return call( host, port, path, fc::variant(v) ); }

eos::chain_apis::read_only::get_info_results get_info() {
  return call(host, port, get_info_func ).as<eos::chain_apis::read_only::get_info_results>();
}

fc::variant push_transaction( SignedTransaction& trx ) {
    auto info = get_info();
    trx.expiration = info.head_block_time + 100; //chain.head_block_time() + 100;
    transaction_helpers::set_reference_block(trx, info.head_block_id);
    boost::sort( trx.scope );

    return call( push_txn_func, trx );
}


void create_account( const vector<string>& cmd_line ) {

      Name creator(cmd_line[2]);
      Name newaccount(cmd_line[3]);
      Name eosaccnt(config::EosContractName);
      Name staked("staked");

      public_key_type owner_key(cmd_line[4]);
      public_key_type active_key(cmd_line[5]);

      auto owner_auth   = eos::chain::Authority{1, {{owner_key, 1}}, {}};
      auto active_auth  = eos::chain::Authority{1, {{active_key, 1}}, {}};
      auto recovery_auth = eos::chain::Authority{1, {}, {{{creator, "active"}, 1}}};

      uint64_t deposit = 1;

      SignedTransaction trx;
      trx.scope = sort_names({creator,eosaccnt});
      transaction_helpers::emplace_message(trx, config::EosContractName, vector<types::AccountPermission>{{creator,"active"}}, "newaccount",
                         types::newaccount{creator, newaccount, owner_auth,
                                           active_auth, recovery_auth, deposit});

      std::cout << fc::json::to_pretty_string( push_transaction(trx) ) << std::endl;


}

static const char escape = '\\';
static const string quote = "'\"";

bool is_quote (const string &line, size_t pos) {
  if (pos == 0 || pos == string::npos)
    return (pos == 0);

  if (line[pos-1] == escape) {
    return (pos == 1) ? false : line[pos-2] == escape;
  }
return true;
}

/**
 * read in the next line of text and split into whitespace separated
 * words, allowing for quoted strings which can span multiple lines
 * if necessary
 **/

char read_line_and_split (vector<string> &cmd_line, char tic) {
  string line;
  bool in_quote = (tic != '\0');
  std::getline(std::cin,line);
  size_t tic_pos = 0;
  size_t quote_end = 0;
  if (in_quote) {
    size_t last = cmd_line.size() - 1;
    cmd_line[last] += "\n";
    for (quote_end = line.find (tic);
         !is_quote (line,quote_end) && quote_end != string::npos;
         quote_end = line.find (tic, quote_end + 1));

    if (quote_end == string::npos) {
      cmd_line[last] += line;
      return tic;
    }

    in_quote = false;
    if (quote_end > 0) {
      cmd_line[last] += line.substr(0, quote_end);
    }
    tic_pos = ++quote_end;
    tic = '\0';
  }
  for (; tic_pos < line.length(); tic_pos++) {
    if (line[tic_pos] == escape) {
      line.erase(tic_pos,1);
      continue;
    }
    if (quote.find (line[tic_pos]) != string::npos) {
      tic = line[tic_pos];
      vector<string> wrap;
      string sub = line.substr(quote_end,tic_pos-1);
      split (wrap, sub, boost::is_any_of(" \t"));
      for (auto &w : wrap) {
        cmd_line.emplace_back("");
        cmd_line[cmd_line.size()-1].swap(w);
      }

      for (quote_end = line.find (tic,tic_pos+1);
           quote_end != string::npos && !is_quote(line,quote_end);
           quote_end = line.find(tic,quote_end + 1));

      if (quote_end == string::npos) {
        in_quote = true;
        cmd_line.push_back(line.substr (tic_pos+1));
        break;
      }
      if (quote_end == tic_pos+1) {
        cmd_line.push_back("");
      }
      else {
        size_t len = quote_end - (tic_pos+1);
        cmd_line.push_back(line.substr (tic_pos+1, len));
      }
      tic_pos = quote_end++;
    }
  }

  if (in_quote) {
    return tic;
  }
  if (quote_end == 0 && cmd_line.size() == 0) {
    split (cmd_line, line, boost::is_any_of(" \t"));
  }
  else {
    vector<string> wrap;
    string sub = line.substr(quote_end,tic_pos-1);
    split (wrap, sub, boost::is_any_of(" \t"));
    for (auto &w : wrap) {
      if (w.length()) {
        cmd_line.emplace_back("");
        cmd_line[cmd_line.size()-1].swap(w);
      }
    }
  }
  return '\0';
}



int send_command (const vector<string> &cmd_line)
{
  const auto& command = cmd_line[0];
  if( command == "help" ) {
    std::cout << "Command list: info, block, exec, account, push-trx, setcode, transfer, create, import, unlock, lock, do, transaction, and transactions\n";
    return -1;
  }

  else if( command == "info" ) {
    std::cout << fc::json::to_pretty_string( get_info() ) << std::endl;
  }
  else if( command == "block" ) {
    FC_ASSERT( cmd_line.size() == 2 );
    auto arg=fc::mutable_variant_object( "block_num_or_id", cmd_line[1]);
    std::cout << fc::json::to_pretty_string( call (get_block_func, arg)) << std::endl;
  }
  else if( command == "exec" ) {
    FC_ASSERT( cmd_line.size() >= 6 );
    Name code   = cmd_line[1];
    Name action = cmd_line[2];
    auto& json   = cmd_line[3];
    auto arg= fc::mutable_variant_object( "code", code )("action",action)("args", fc::json::from_string(json));
    auto result = call( json_to_bin_func, arg);


    SignedTransaction trx;
    trx.messages.resize(1);
    auto& msg = trx.messages.back();
    msg.code = code;
    msg.type = action;
    msg.authorization = fc::json::from_string( cmd_line[5] ).as<vector<types::AccountPermission>>();
    msg.data = result.get_object()["binargs"].as<Bytes>();
    trx.scope = fc::json::from_string( cmd_line[4] ).as<vector<Name>>();

    auto trx_result = push_transaction( trx );
    std::cout << fc::json::to_pretty_string( trx_result ) << std::endl;

  } else if( command == "account" ) {
    FC_ASSERT( cmd_line.size() == 2 );
    std::cout << fc::json::to_pretty_string( call( get_account_func,
                                                   fc::mutable_variant_object( "name", cmd_line[1] ) ) ) << std::endl;
  }
  else if( command == "push-trx" ) {
    auto trx_result = call (push_txn_func, fc::json::from_string( cmd_line[1]));
    std::cout << fc::json::to_pretty_string(trx_result) << std::endl;
  } else if ( command == "setcode" ) {
    if( cmd_line.size() == 1 ) {
      std::cout << "usage: "<< program << " " << command <<" ACCOUNT FILE.WAST FILE.ABI" << std::endl;
      return -1;
    }
    Name account(cmd_line[1]);
    const auto& wast_file = cmd_line[2];
    std::string wast;

    FC_ASSERT( fc::exists(wast_file) );
    fc::read_file_contents( wast_file, wast );
    auto wasm = assemble_wast( wast );



    types::setcode handler;
    handler.account = account;
    handler.code.resize(wasm.size());
    memcpy( handler.code.data(), wasm.data(), wasm.size() );

    if( cmd_line.size() == 4 ) {
      handler.abi = fc::json::from_file( cmd_line[3] ).as<types::Abi>();
    }

    SignedTransaction trx;
    trx.scope = { config::EosContractName, account };
    transaction_helpers::emplace_message(trx,  config::EosContractName, vector<types::AccountPermission>{{account,"active"}},
                        "setcode", handler );

    std::cout << fc::json::to_pretty_string( push_transaction(trx)  ) << std::endl;

  } else if( command == "transfer" ) {
    FC_ASSERT( cmd_line.size() == 4 );

    Name sender(cmd_line[1]);
    Name recipient(cmd_line[2]);
    uint64_t amount = fc::variant(cmd_line[3]).as_uint64();

    SignedTransaction trx;
    trx.scope = sort_names({sender,recipient});
    transaction_helpers::emplace_message(trx, config::EosContractName, vector<types::AccountPermission>{{sender,"active"}}, "transfer",
                       types::transfer{sender, recipient, amount});
    auto info = get_info();
    trx.expiration = info.head_block_time + 100; //chain.head_block_time() + 100;
    transaction_helpers::set_reference_block(trx, info.head_block_id);

    std::cout << fc::json::to_pretty_string( call( push_txn_func, trx )) << std::endl;
  }
  else if (command == "create" ) {
    if( cmd_line[1] == "account" ) {
      if( cmd_line.size() < 6 ) {
        std::cerr << "usage: " << program << " create account CREATOR NEWACCOUNT OWNERKEY ACTIVEKEY\n";
        return -1;
      }
      create_account( cmd_line );
    }
    else if( cmd_line[1] == "key" ) {
      auto priv = fc::ecc::private_key::generate();
      auto pub = public_key_type( priv.get_public_key() );

      std::cout << "public: " << string(pub) <<"\n";
      std::cout << "private: " << key_to_wif(priv.get_secret()) << std::endl;
    }
    else {
      std::cerr << "create doesn't recognize object " << cmd_line[1] << std::endl;
    }
  } else if( command == "import" ) {
    if( cmd_line[1] == "key" ) {
      auto secret = wif_to_key( cmd_line[2] ); //fc::variant( cmd_line[2] ).as<fc::ecc::private_key_secret>();
      if( !secret ) {
        std::cerr << "invalid WIF private key" << std::endl;;
        return -1;
      }
      auto priv = fc::ecc::private_key::regenerate(*secret);
      auto pub = public_key_type( priv.get_public_key() );
      std::cout << "public: " << string(pub) << std::endl;;
    }
  }else if( command == "unlock" ) {

  } else if( command == "lock" ) {

  } else if( command == "do" ) {

  } else if( command == "transaction" ) {
     if( cmd_line.size() != 2 )
     {
        std::cerr << "usage: " << program << " transaction TRANSACTION_ID\n";
        return -1;
     }
     auto arg= fc::mutable_variant_object( "transaction_id", cmd_line[1]);
     std::cout << fc::json::to_pretty_string( call( get_transaction_func, arg) ) << std::endl;
  } else if( command == "transactions" ) {
     if( cmd_line.size() < 2 || cmd_line.size() > 4 )
     {
        std::cerr << "usage: " << program << " transactions ACCOUNT_TO_LOOKUP [SKIP_TO_SEQUENCE [NUMBER_OF_SEQUENCES_TO_RETURN]]\n";
        return -1;
     }
     chain::AccountName account_name(cmd_line[1]);
     auto arg = (cmd_line.size() == 2)
           ? fc::mutable_variant_object( "account_name", account_name)
           : (cmd_line.size() == 3)
             ? fc::mutable_variant_object( "account_name", account_name)("skip_seq", cmd_line[2])
             : fc::mutable_variant_object( "account_name", account_name)("skip_seq", cmd_line[2])("num_seq", cmd_line[3]);
     std::cout << fc::json::to_pretty_string( call( get_transactions_func, arg) ) << std::endl;
  }
  return 0;
}


/**
 *   Usage:
 *
 *   eosc - [optional command] < script
 *     or *   eocs create wallet walletname  ***PASS1*** ***PASS2***
 *   eosc unlock walletname  ***PASSWORD***
 *   eosc create wallet walletname  ***PASS1*** ***PASS2***
 *   eosc unlock walletname  ***PASSWORD***
 *   eosc wallets -> prints list of wallets with * next to currently unlocked
 *   eosc keys -> prints list of private keys
 *   eosc importkey privatekey -> loads keys
 *   eosc accounts -> prints list of accounts that reference key
 *   eosc lock
 *   eosc do contract action { from to amount }
 *   eosc transfer from to amount memo  => aliaze for eosc
 *   eosc create account creator
 *
 *  if reading from a script all lines are allE parsed before any are sent
 *  reading from the console, lines are acted on directly.
 */

int main( int argc, char** argv ) {
  vector<string> cmd;
  bool is_cmd=false;
  bool from_stdin=false;
  bool batch=false;
  uint32_t cycles=1;
  vector<vector<string> > script;
  program = argv[0];

  for( uint32_t i = 1; i < argc; ++i ) {
    is_cmd |= (*argv[i] != '-');
    if (!is_cmd) {
      from_stdin |= (*(argv[i]+1) == '\0');
      if (!from_stdin) {
        // TODO: parse arguments, --batch=<1|0> --cycles=<n> --input=<1|0>
      }
    }
    else {
      cmd.push_back(string(argv[i]));
    }
  }

  try {
    if (cmd.size()) {
      int res = 0;
      if ((res = send_command(cmd)) != 0) {
        return res;
      }
    }
  } catch ( const fc::exception& e ) {
    std::cerr << e.to_detail_string() << std::endl;
    return 0;
  }

  bool console = isatty(0);

  if (from_stdin) {
    char tic = '\0';
    vector<string> cmd_line;
    if (console) {
      std::cout << "press ^d when done" << std::endl;
    }
    while (std::cin.good()) {
      if (tic == '\0') {
        cmd_line.clear();
        if (console) {
          std::cout << "enter command: " << std::flush;
        }
      }

      tic = read_line_and_split(cmd_line, tic);
      if (tic != '\0') {
        continue;
      }

      if (!console || batch) {
        script.push_back (cmd_line);
      }
      else {
        send_command (cmd_line);
      }
    }
  }

  try {
    int res = 0;
    while (cycles--) {
      for (auto scmd : script) {
        if ((res = send_command (scmd)) != 0) {
          return res;
        }
      }
    }
  } catch ( const fc::exception& e ) {
    std::cerr << e.to_detail_string() << std::endl;
  }
  return 0;
}
