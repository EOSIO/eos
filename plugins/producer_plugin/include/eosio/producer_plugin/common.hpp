#pragma once

#include <boost/multi_index/hashed_index_fwd.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index_container.hpp>

#include <eosio/chain/controller.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/types.hpp>

#include <fc/io/json.hpp>

#undef FC_LOG_AND_DROP
#define LOG_AND_DROP()                                                                 \
catch ( const eosio::chain::guard_exception& e ) {                                     \
      chain_plugin::handle_guard_exception(e);                                         \
   } catch ( const std::bad_alloc& ) {                                                 \
      chain_plugin::handle_bad_alloc();                                                \
   } catch ( boost::interprocess::bad_alloc& ) {                                       \
      chain_plugin::handle_db_exhaustion();                                            \
   } catch( fc::exception& er ) {                                                      \
      wlog( "${details}", ("details",er.to_detail_string()) );                         \
   } catch( const std::exception& e ) {                                                \
      fc::exception fce(                                                               \
                FC_LOG_MESSAGE( warn, "std::exception: ${what}: ",("what",e.what()) ), \
                fc::std_exception_code,                                                \
                BOOST_CORE_TYPEID(e).name(),                                           \
                e.what() ) ;                                                           \
      wlog( "${details}", ("details",fce.to_detail_string()) );                        \
   } catch( ... ) {                                                                    \
      fc::unhandled_exception e(                                                       \
                FC_LOG_MESSAGE( warn, "unknown: ",  ),                                 \
                std::current_exception() );                                            \
      wlog( "${details}", ("details",e.to_detail_string()) );                          \
   }

#define LOAD_VALUE_SET(options, op_name, container)                                       \
if( options.count(op_name) ) {                                                            \
   const std::vector<std::string>& ops = options[op_name].as<std::vector<std::string>>(); \
   for( const auto& v : ops ) {                                                           \
      container.emplace( eosio::chain::name( v ) );                                       \
   }                                                                                      \
}

namespace eosio {

enum class pending_block_mode {
   producing,
   speculating
};

struct transaction_id_with_expiry {
   eosio::chain::transaction_id_type trx_id;
   fc::time_point                    expiry;
};

struct by_expiry;
struct by_height;
struct by_id;

using transaction_id_with_expiry_index = boost::multi_index::multi_index_container<
   transaction_id_with_expiry,
   boost::multi_index::indexed_by<
      boost::multi_index::hashed_unique<boost::multi_index::tag<by_id>, BOOST_MULTI_INDEX_MEMBER(transaction_id_with_expiry, eosio::chain::transaction_id_type, trx_id)>,
      boost::multi_index::ordered_non_unique<boost::multi_index::tag<by_expiry>, BOOST_MULTI_INDEX_MEMBER(transaction_id_with_expiry, fc::time_point, expiry)>
   >
>;

const fc::string logger_name("producer_plugin");
fc::logger _log;

const fc::string trx_trace_logger_name("transaction_tracing");
fc::logger _trx_trace_log;

namespace {
   bool exception_is_exhausted(const fc::exception& e, bool deadline_is_subjective) {
      auto code = e.code();
      return (code == eosio::chain::block_cpu_usage_exceeded::code_value) ||
             (code == eosio::chain::block_net_usage_exceeded::code_value) ||
             (code == eosio::chain::deadline_exception::code_value && deadline_is_subjective);
   }
} // namespace

template<typename T>
T dejsonify(const std::string& s) {
   return fc::json::from_string(s).as<T>();
}

static auto make_debug_time_logger() {
   auto start = fc::time_point::now();
   return fc::make_scoped_exit([=](){
      fc_dlog(_log, "Signing took ${ms}us", ("ms", fc::time_point::now() - start) );
   });
}

static auto maybe_make_debug_time_logger() -> fc::optional<decltype(make_debug_time_logger())> {
   if (_log.is_enabled( fc::log_level::debug ) ){
      return make_debug_time_logger();
   } else {
      return {};
   }
}

void new_chain_banner(const eosio::chain::controller& db) {
   std::cerr << "\n"
      "*******************************\n"
      "*                             *\n"
      "*   ------ NEW CHAIN ------   *\n"
      "*   -  Welcome to EOSIO!  -   *\n"
      "*   -----------------------   *\n"
      "*                             *\n"
      "*******************************\n"
      "\n";

   if( db.head_block_state()->header.timestamp.to_time_point() < (fc::time_point::now() - fc::milliseconds(200 * eosio::chain::config::block_interval_ms)))
   {
      std::cerr << "Your genesis seems to have an old timestamp\n"
         "Please consider using the --genesis-timestamp option to give your genesis a recent timestamp\n"
         "\n"
         ;
   }
   return;
}

}
