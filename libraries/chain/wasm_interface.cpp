/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <boost/function.hpp>
#include <boost/multiprecision/cpp_bin_float.hpp>
#include <eos/chain/wasm_interface.hpp>
#include <eos/chain/chain_controller.hpp>
#include <eos/chain/rate_limiting_object.hpp>
#include "Platform/Platform.h"
#include "WAST/WAST.h"
#include "Runtime/Runtime.h"
#include "Runtime/Linker.h"
#include "Runtime/Intrinsics.h"
#include "IR/Module.h"
#include "IR/Operators.h"
#include "IR/Validate.h"
#include <eos/chain/key_value_object.hpp>
#include <eos/chain/account_object.hpp>
#include <eos/chain/balance_object.hpp>
#include <eos/chain/staked_balance_objects.hpp>
#include <eos/types/abi_serializer.hpp>
#include <chrono>
#include <boost/lexical_cast.hpp>
#include <fc/utf8.hpp>

namespace eosio { namespace chain {
   using namespace IR;
   using namespace Runtime;
   typedef boost::multiprecision::cpp_bin_float_50 DOUBLE;
   const uint32_t bytes_per_mbyte = 1024 * 1024;

   class wasm_memory
   {
   public:
      explicit wasm_memory(wasm_interface& interface);
      wasm_memory(const wasm_memory&) = delete;
      wasm_memory(wasm_memory&&) = delete;
      ~wasm_memory();
      U32 sbrk(I32 num_bytes);
   private:
      static U32 limit_32bit_address(Uptr address);

      static const U32 _max_memory = 1024 * 1024;
      wasm_interface& _wasm_interface;
      Uptr _num_pages;
      const U32 _min_bytes;
      U32 _num_bytes;
   };

   // account.h/hpp expected account API balance interchange format
   // must match account.hpp account_balance definition
   PACKED_STRUCT(
   struct account_balance
   {
      /**
      * Name of the account who's balance this is
      */
      account_name account;

      /**
      * Balance for this account
      */
      asset eos_balance;

      /**
      * Staked balance for this account
      */
      asset staked_balance;

      /**
      * Unstaking balance for this account
      */
      asset unstaking_balance;

      /**
      * Time at which last unstaking occurred for this account
      */
      time last_unstaking_time;
   })

   wasm_interface::wasm_interface() {
   }

   wasm_interface::key_type wasm_interface::to_key_type(const types::type_name& type_name)
   {
      if ("str" == type_name)
         return str;
      if ("i64" == type_name)
         return i64;
      if ("i128i128" == type_name)
         return i128i128;
      if ("i64i64i64" == type_name)
         return i64i64i64;

      return invalid_key_type;
   }

   std::string wasm_interface::to_type_name(key_type key_type)
   {
      switch (key_type)
      {
      case str:
         return "str";
      case i64:
         return "i64";
      case i128i128:
         return "i128i128";
      case i64i64i64:
         return "i64i64i64";
      default:
         return std::string("<invalid key type - ") + boost::lexical_cast<std::string>(int(key_type)) + ">";
      }
   }

#ifdef NDEBUG
   const int CHECKTIME_LIMIT = 3000;
#else
   const int CHECKTIME_LIMIT = 18000;
#endif

   void checktime(int64_t duration, uint32_t checktime_limit)
   {
      if (duration > checktime_limit) {
         wlog("checktime called ${d}", ("d", duration));
         throw checktime_exceeded();
      }
   }

DEFINE_INTRINSIC_FUNCTION0(env,checktime,checktime,none) {
   checktime(wasm_interface::get().current_execution_time(), wasm_interface::get().checktime_limit);
}

   template <typename Function, typename KeyType, int numberOfKeys>
   int32_t validate(int32_t valueptr, int32_t valuelen, Function func) {

      static const uint32_t keylen = numberOfKeys*sizeof(KeyType);

      FC_ASSERT( valuelen >= keylen, "insufficient data passed" );

      auto& wasm  = wasm_interface::get();
      FC_ASSERT( wasm.current_apply_context, "no apply context found" );

      char* value = memoryArrayPtr<char>( wasm.current_memory, valueptr, valuelen );
      KeyType*  keys = reinterpret_cast<KeyType*>(value);
      
      valuelen -= keylen;
      value    += keylen;

      return func(wasm.current_apply_context, keys, value, valuelen);
   }

   template <typename Function>
   int32_t validate_str(int32_t keyptr, int32_t keylen, int32_t valueptr, int32_t valuelen, Function func) {

      auto& wasm  = wasm_interface::get();
      FC_ASSERT( wasm.current_apply_context, "no apply context found" );

      char* key   = memoryArrayPtr<char>( wasm.current_memory, keyptr, keylen );
      char* value = memoryArrayPtr<char>( wasm.current_memory, valueptr, valuelen );

      std::string keys(key, keylen);

      return func(wasm.current_apply_context, &keys, value, valuelen);
   }

   int64_t round_to_byte_boundary(int64_t bytes)
   {
      const unsigned int byte_boundary = sizeof(uint64_t);
      int64_t remainder = bytes % byte_boundary;
      if (remainder > 0)
         bytes += byte_boundary - remainder;
      return bytes;
   }

#define VERIFY_TABLE(TYPE) \
   const auto table_name = name(table); \
   auto& wasm  = wasm_interface::get(); \
   if (wasm.table_key_types) \
   { \
      auto table_key = wasm.table_key_types->find(table_name); \
      if (table_key == wasm.table_key_types->end()) \
      { \
         FC_ASSERT(!wasm.tables_fixed, "abi did not define table ${t}", ("t", table_name)); \
         wasm.table_key_types->emplace(std::make_pair(table_name,wasm_interface::TYPE)); \
      } \
      else \
      { \
         FC_ASSERT(wasm_interface::TYPE == table_key->second, "abi definition for ${table} expects \"${type}\", but code is requesting \"" #TYPE "\"", ("table",table_name)("type",wasm_interface::to_type_name(table_key->second))); \
      } \
   }

#define READ_RECORD(READFUNC, INDEX, SCOPE) \
   auto lambda = [&](apply_context* ctx, INDEX::value_type::key_type* keys, char *data, uint32_t datalen) -> int32_t { \
      auto res = ctx->READFUNC<INDEX, SCOPE>( name(scope), name(code), table_name, keys, data, datalen); \
      if (res >= 0) res += INDEX::value_type::number_of_keys*sizeof(INDEX::value_type::key_type); \
      return res; \
   }; \
   return validate<decltype(lambda), INDEX::value_type::key_type, INDEX::value_type::number_of_keys>(valueptr, valuelen, lambda);

#define UPDATE_RECORD(UPDATEFUNC, INDEX, DATASIZE) \
   auto lambda = [&](apply_context* ctx, INDEX::value_type::key_type* keys, char *data, uint32_t datalen) -> int32_t { \
      return ctx->UPDATEFUNC<INDEX::value_type>( name(scope), name(ctx->code.value), table_name, keys, data, datalen); \
   }; \
   const int32_t ret = validate<decltype(lambda), INDEX::value_type::key_type, INDEX::value_type::number_of_keys>(valueptr, DATASIZE, lambda);

#define DEFINE_RECORD_UPDATE_FUNCTIONS(OBJTYPE, INDEX, KEY_SIZE) \
   DEFINE_INTRINSIC_FUNCTION4(env,store_##OBJTYPE,store_##OBJTYPE,i32,i64,scope,i64,table,i32,valueptr,i32,valuelen) { \
      VERIFY_TABLE(OBJTYPE) \
      UPDATE_RECORD(store_record, INDEX, valuelen); \
      /* ret is -1 if record created, or else it is the length of the data portion of the originally stored */ \
      /* structure (it does not include the key portion */ \
      const bool created = (ret == -1); \
      int64_t& storage = wasm_interface::get().table_storage; \
      if (created) \
         storage += round_to_byte_boundary(valuelen) + wasm_interface::get().row_overhead_db_limit_bytes; \
      else \
         /* need to calculate the difference between the original rounded byte size and the new rounded byte size */ \
         storage += round_to_byte_boundary(valuelen) - round_to_byte_boundary(KEY_SIZE + ret); \
\
      EOS_ASSERT(storage <= (wasm_interface::get().per_code_account_max_db_limit_mbytes * bytes_per_mbyte), \
                 tx_code_db_limit_exceeded, \
                 "Database limit exceeded for account=${name}",("name", name(wasm_interface::get().current_apply_context->code.value))); \
      return created ? 1 : 0; \
   } \
   DEFINE_INTRINSIC_FUNCTION4(env,update_##OBJTYPE,update_##OBJTYPE,i32,i64,scope,i64,table,i32,valueptr,i32,valuelen) { \
      VERIFY_TABLE(OBJTYPE) \
      UPDATE_RECORD(update_record, INDEX, valuelen); \
      /* ret is -1 if record created, or else it is the length of the data portion of the originally stored */ \
      /* structure (it does not include the key portion */ \
      if (ret == -1) return 0; \
      int64_t& storage = wasm_interface::get().table_storage; \
      /* need to calculate the difference between the original rounded byte size and the new rounded byte size */ \
      storage += round_to_byte_boundary(valuelen) - round_to_byte_boundary(KEY_SIZE + ret); \
      \
      EOS_ASSERT(storage <= (wasm_interface::get().per_code_account_max_db_limit_mbytes * bytes_per_mbyte), \
                 tx_code_db_limit_exceeded, \
                 "Database limit exceeded for account=${name}",("name", name(wasm_interface::get().current_apply_context->code.value))); \
      return 1; \
   } \
   DEFINE_INTRINSIC_FUNCTION3(env,remove_##OBJTYPE,remove_##OBJTYPE,i32,i64,scope,i64,table,i32,valueptr) { \
      VERIFY_TABLE(OBJTYPE) \
      UPDATE_RECORD(remove_record, INDEX, sizeof(typename INDEX::value_type::key_type)*INDEX::value_type::number_of_keys); \
      /* ret is -1 if record created, or else it is the length of the data portion of the originally stored */ \
      /* structure (it does not include the key portion */ \
      if (ret == -1) return 0; \
      int64_t& storage = wasm_interface::get().table_storage; \
      storage -= round_to_byte_boundary(KEY_SIZE + ret) + wasm_interface::get().row_overhead_db_limit_bytes; \
      \
      EOS_ASSERT(storage <= (wasm_interface::get().per_code_account_max_db_limit_mbytes * bytes_per_mbyte), \
                 tx_code_db_limit_exceeded, \
                 "Database limit exceeded for account=${name}",("name", name(wasm_interface::get().current_apply_context->code.value))); \
      return 1; \
   }

#define DEFINE_RECORD_READ_FUNCTION(OBJTYPE, ACTION, FUNCPREFIX, INDEX, SCOPE) \
   DEFINE_INTRINSIC_FUNCTION5(env,ACTION##_##FUNCPREFIX##OBJTYPE,ACTION##_##FUNCPREFIX##OBJTYPE,i32,i64,scope,i64,code,i64,table,i32,valueptr,i32,valuelen) { \
      VERIFY_TABLE(OBJTYPE) \
      READ_RECORD(ACTION##_record, INDEX, SCOPE); \
   }

#define DEFINE_RECORD_READ_FUNCTIONS(OBJTYPE, FUNCPREFIX, INDEX, SCOPE) \
   DEFINE_RECORD_READ_FUNCTION(OBJTYPE, load, FUNCPREFIX, INDEX, SCOPE) \
   DEFINE_RECORD_READ_FUNCTION(OBJTYPE, front, FUNCPREFIX, INDEX, SCOPE) \
   DEFINE_RECORD_READ_FUNCTION(OBJTYPE, back, FUNCPREFIX, INDEX, SCOPE) \
   DEFINE_RECORD_READ_FUNCTION(OBJTYPE, next, FUNCPREFIX, INDEX, SCOPE) \
   DEFINE_RECORD_READ_FUNCTION(OBJTYPE, previous, FUNCPREFIX, INDEX, SCOPE) \
   DEFINE_RECORD_READ_FUNCTION(OBJTYPE, lower_bound, FUNCPREFIX, INDEX, SCOPE) \
   DEFINE_RECORD_READ_FUNCTION(OBJTYPE, upper_bound, FUNCPREFIX, INDEX, SCOPE)

DEFINE_RECORD_UPDATE_FUNCTIONS(i64, key_value_index, 8);
DEFINE_RECORD_READ_FUNCTIONS(i64,,key_value_index, by_scope_primary);
      
DEFINE_RECORD_UPDATE_FUNCTIONS(i128i128, key128x128_value_index, 32);
DEFINE_RECORD_READ_FUNCTIONS(i128i128, primary_,   key128x128_value_index, by_scope_primary);
DEFINE_RECORD_READ_FUNCTIONS(i128i128, secondary_, key128x128_value_index, by_scope_secondary);

DEFINE_RECORD_UPDATE_FUNCTIONS(i64i64i64, key64x64x64_value_index, 24);
DEFINE_RECORD_READ_FUNCTIONS(i64i64i64, primary_,   key64x64x64_value_index, by_scope_primary);
DEFINE_RECORD_READ_FUNCTIONS(i64i64i64, secondary_, key64x64x64_value_index, by_scope_secondary);
DEFINE_RECORD_READ_FUNCTIONS(i64i64i64, tertiary_,  key64x64x64_value_index, by_scope_tertiary);


#define UPDATE_RECORD_STR(FUNCTION) \
  VERIFY_TABLE(str) \
  auto lambda = [&](apply_context* ctx, std::string* keys, char *data, uint32_t datalen) -> int32_t { \
    return ctx->FUNCTION<keystr_value_object>( name(scope), name(ctx->code.value), table_name, keys, data, datalen); \
  }; \
  const int32_t ret = validate_str<decltype(lambda)>(keyptr, keylen, valueptr, valuelen, lambda);

#define READ_RECORD_STR(FUNCTION) \
  VERIFY_TABLE(str) \
  auto lambda = [&](apply_context* ctx, std::string* keys, char *data, uint32_t datalen) -> int32_t { \
    auto res = ctx->FUNCTION<keystr_value_index, by_scope_primary>( name(scope), name(code), table_name, keys, data, datalen); \
    return res; \
  }; \
  return validate_str<decltype(lambda)>(keyptr, keylen, valueptr, valuelen, lambda);

DEFINE_INTRINSIC_FUNCTION6(env,store_str,store_str,i32,i64,scope,i64,table,i32,keyptr,i32,keylen,i32,valueptr,i32,valuelen) {
   UPDATE_RECORD_STR(store_record)
   const bool created = (ret == -1);
   auto& storage = wasm_interface::get().table_storage;
   if (created)
   {
      storage += round_to_byte_boundary(keylen + valuelen) + wasm_interface::get().row_overhead_db_limit_bytes;
   }
   else
      // need to calculate the difference between the original rounded byte size and the new rounded byte size
      storage += round_to_byte_boundary(keylen + valuelen) - round_to_byte_boundary(keylen + ret);

   EOS_ASSERT(storage <= (wasm_interface::get().per_code_account_max_db_limit_mbytes * bytes_per_mbyte),
              tx_code_db_limit_exceeded,
              "Database limit exceeded for account=${name}",("name", name(wasm_interface::get().current_apply_context->code.value)));

   return created ? 1 : 0;
}
DEFINE_INTRINSIC_FUNCTION6(env,update_str,update_str,i32,i64,scope,i64,table,i32,keyptr,i32,keylen,i32,valueptr,i32,valuelen) {
   UPDATE_RECORD_STR(update_record)
   if (ret == -1) return 0;
   auto& storage = wasm_interface::get().table_storage;
   // need to calculate the difference between the original rounded byte size and the new rounded byte size
   storage += round_to_byte_boundary(keylen + valuelen) - round_to_byte_boundary(keylen + ret);

   EOS_ASSERT(storage <= (wasm_interface::get().per_code_account_max_db_limit_mbytes * bytes_per_mbyte),
              tx_code_db_limit_exceeded,
              "Database limit exceeded for account=${name}",("name", name(wasm_interface::get().current_apply_context->code.value)));
   return 1;
}
DEFINE_INTRINSIC_FUNCTION4(env,remove_str,remove_str,i32,i64,scope,i64,table,i32,keyptr,i32,keylen) {
   int32_t valueptr=0, valuelen=0;
   UPDATE_RECORD_STR(remove_record)
   if (ret == -1) return 0;
   auto& storage = wasm_interface::get().table_storage;
   storage -= round_to_byte_boundary(keylen + ret) + wasm_interface::get().row_overhead_db_limit_bytes;

   EOS_ASSERT(storage <= (wasm_interface::get().per_code_account_max_db_limit_mbytes * bytes_per_mbyte),
              tx_code_db_limit_exceeded,
              "Database limit exceeded for account=${name}",("name", name(wasm_interface::get().current_apply_context->code.value)));
   return 1;
}

DEFINE_INTRINSIC_FUNCTION7(env,load_str,load_str,i32,i64,scope,i64,code,i64,table,i32,keyptr,i32,keylen,i32,valueptr,i32,valuelen) {
  READ_RECORD_STR(load_record)
}
DEFINE_INTRINSIC_FUNCTION5(env,front_str,front_str,i32,i64,scope,i64,code,i64,table,i32,valueptr,i32,valuelen) {
  int32_t keyptr=0, keylen=0;
  READ_RECORD_STR(front_record)
}
DEFINE_INTRINSIC_FUNCTION5(env,back_str,back_str,i32,i64,scope,i64,code,i64,table,i32,valueptr,i32,valuelen) {
  int32_t keyptr=0, keylen=0;
  READ_RECORD_STR(back_record)
}
DEFINE_INTRINSIC_FUNCTION7(env,next_str,next_str,i32,i64,scope,i64,code,i64,table,i32,keyptr,i32,keylen,i32,valueptr,i32,valuelen) {
  READ_RECORD_STR(next_record)
}
DEFINE_INTRINSIC_FUNCTION7(env,previous_str,previous_str,i32,i64,scope,i64,code,i64,table,i32,keyptr,i32,keylen,i32,valueptr,i32,valuelen) {
  READ_RECORD_STR(previous_record)
}
DEFINE_INTRINSIC_FUNCTION7(env,lower_bound_str,lower_bound_str,i32,i64,scope,i64,code,i64,table,i32,keyptr,i32,keylen,i32,valueptr,i32,valuelen) {
  READ_RECORD_STR(lower_bound_record)
}
DEFINE_INTRINSIC_FUNCTION7(env,upper_bound_str,upper_bound_str,i32,i64,scope,i64,code,i64,table,i32,keyptr,i32,keylen,i32,valueptr,i32,valuelen) {
  READ_RECORD_STR(upper_bound_record)
}

DEFINE_INTRINSIC_FUNCTION3(env, assert_is_utf8,assert_is_utf8,none,i32,dataptr,i32,datalen,i32,msg) {
  auto& wasm  = wasm_interface::get();
  auto  mem   = wasm.current_memory;

  const char* str = &memoryRef<const char>( mem, dataptr );
  const bool test = fc::is_utf8(std::string( str, datalen ));

  FC_ASSERT( test, "assertion failed: ${s}", ("s",msg) );
}

DEFINE_INTRINSIC_FUNCTION3(env, assert_sha256,assert_sha256,none,i32,dataptr,i32,datalen,i32,hash) {
   FC_ASSERT( datalen > 0 );

   auto& wasm  = wasm_interface::get();
   auto  mem          = wasm.current_memory;

   char* data = memoryArrayPtr<char>( mem, dataptr, datalen );
   const auto& v = memoryRef<fc::sha256>( mem, hash );

   auto result = fc::sha256::hash( data, datalen );
   FC_ASSERT( result == v, "hash miss match" );
}

DEFINE_INTRINSIC_FUNCTION3(env,sha256,sha256,none,i32,dataptr,i32,datalen,i32,hash) {
   FC_ASSERT( datalen > 0 );

   auto& wasm  = wasm_interface::get();
   auto  mem          = wasm.current_memory;

   char* data = memoryArrayPtr<char>( mem, dataptr, datalen );
   auto& v = memoryRef<fc::sha256>( mem, hash );
   v  = fc::sha256::hash( data, datalen );
}

DEFINE_INTRINSIC_FUNCTION2(env,multeq_i128,multeq_i128,none,i32,self,i32,other) {
   auto& wasm  = wasm_interface::get();
   auto  mem   = wasm.current_memory;
   auto& v = memoryRef<unsigned __int128>( mem, self );
   const auto& o = memoryRef<const unsigned __int128>( mem, other );
   v *= o;
}

DEFINE_INTRINSIC_FUNCTION2(env,diveq_i128,diveq_i128,none,i32,self,i32,other) {
   auto& wasm  = wasm_interface::get();
   auto  mem          = wasm.current_memory;
   auto& v = memoryRef<unsigned __int128>( mem, self );
   const auto& o = memoryRef<const unsigned __int128>( mem, other );
   FC_ASSERT( o != 0, "divide by zero" );
   v /= o;
}

DEFINE_INTRINSIC_FUNCTION2(env,double_add,double_add,i64,i64,a,i64,b) {
   DOUBLE c = DOUBLE(*reinterpret_cast<double *>(&a))
            + DOUBLE(*reinterpret_cast<double *>(&b));
   double res = c.convert_to<double>();
   return *reinterpret_cast<uint64_t *>(&res);
}

DEFINE_INTRINSIC_FUNCTION2(env,double_mult,double_mult,i64,i64,a,i64,b) {
   DOUBLE c = DOUBLE(*reinterpret_cast<double *>(&a))
            * DOUBLE(*reinterpret_cast<double *>(&b));
   double res = c.convert_to<double>();
   return *reinterpret_cast<uint64_t *>(&res);
}

DEFINE_INTRINSIC_FUNCTION2(env,double_div,double_div,i64,i64,a,i64,b) {
   auto divisor = DOUBLE(*reinterpret_cast<double *>(&b));
   FC_ASSERT( divisor != 0, "divide by zero" );

   DOUBLE c = DOUBLE(*reinterpret_cast<double *>(&a)) / divisor;
   double res = c.convert_to<double>();
   return *reinterpret_cast<uint64_t *>(&res);
}

DEFINE_INTRINSIC_FUNCTION2(env,double_lt,double_lt,i32,i64,a,i64,b) {
   return DOUBLE(*reinterpret_cast<double *>(&a))
        < DOUBLE(*reinterpret_cast<double *>(&b));
}

DEFINE_INTRINSIC_FUNCTION2(env,double_eq,double_eq,i32,i64,a,i64,b) {
   return DOUBLE(*reinterpret_cast<double *>(&a))
       == DOUBLE(*reinterpret_cast<double *>(&b));
}

DEFINE_INTRINSIC_FUNCTION2(env,double_gt,double_gt,i32,i64,a,i64,b) {
   return DOUBLE(*reinterpret_cast<double *>(&a))
        > DOUBLE(*reinterpret_cast<double *>(&b));
}

DEFINE_INTRINSIC_FUNCTION1(env,double_to_i64,double_to_i64,i64,i64,a) {
   return DOUBLE(*reinterpret_cast<double *>(&a))
          .convert_to<uint64_t>();
}

DEFINE_INTRINSIC_FUNCTION1(env,i64_to_double,i64_to_double,i64,i64,a) {
   double res = DOUBLE(a).convert_to<double>();
   return *reinterpret_cast<uint64_t *>(&res);
}

DEFINE_INTRINSIC_FUNCTION2(env,get_active_producers,get_active_producers,none,i32,producers,i32,datalen) {
   auto& wasm    = wasm_interface::get();
   auto  mem     = wasm.current_memory;
   types::account_name* dst = memoryArrayPtr<types::account_name>( mem, producers, datalen );
   return wasm_interface::get().current_validate_context->get_active_producers(dst, datalen);
}

DEFINE_INTRINSIC_FUNCTION0(env,now,now,i32) {
   return wasm_interface::get().current_validate_context->controller.head_block_time().sec_since_epoch();
}

DEFINE_INTRINSIC_FUNCTION0(env,current_code,current_code,i64) {
   auto& wasm  = wasm_interface::get();
   return wasm.current_validate_context->code.value;
}

DEFINE_INTRINSIC_FUNCTION1(env,require_auth,require_auth,none,i64,account) {
   wasm_interface::get().current_validate_context->require_authorization( name(account) );
}

DEFINE_INTRINSIC_FUNCTION1(env,require_notice,require_notice,none,i64,account) {
   wasm_interface::get().current_apply_context->require_recipient( account );
}

DEFINE_INTRINSIC_FUNCTION3(env,memcpy,memcpy,i32,i32,dstp,i32,srcp,i32,len) {
   auto& wasm          = wasm_interface::get();
   auto  mem           = wasm.current_memory;
   char* dst           = memoryArrayPtr<char>( mem, dstp, len);
   const char* src     = memoryArrayPtr<const char>( mem, srcp, len );
   FC_ASSERT( len > 0 );

   if( dst > src )
      FC_ASSERT( dst >= (src + len), "overlap of memory range is undefined", ("d",dstp)("s",srcp)("l",len) );
   else
      FC_ASSERT( src >= (dst + len), "overlap of memory range is undefined", ("d",dstp)("s",srcp)("l",len) );

   memcpy( dst, src, uint32_t(len) );
   return dstp;
}

DEFINE_INTRINSIC_FUNCTION3(env,memcmp,memcmp,i32,i32,dstp,i32,srcp,i32,len) {
   auto& wasm          = wasm_interface::get();
   auto  mem           = wasm.current_memory;
   char* dst           = memoryArrayPtr<char>( mem, dstp, len);
   const char* src     = memoryArrayPtr<const char>( mem, srcp, len );
   FC_ASSERT( len > 0 );

   return memcmp( dst, src, uint32_t(len) );
}


DEFINE_INTRINSIC_FUNCTION3(env,memset,memset,i32,i32,rel_ptr,i32,value,i32,len) {
   auto& wasm          = wasm_interface::get();
   auto  mem           = wasm.current_memory;
   char* ptr           = memoryArrayPtr<char>( mem, rel_ptr, len);
   FC_ASSERT( len > 0 );

   memset( ptr, value, len );
   return rel_ptr;
}

DEFINE_INTRINSIC_FUNCTION1(env,sbrk,sbrk,i32,i32,num_bytes) {
   auto& wasm          = wasm_interface::get();

   FC_ASSERT( num_bytes >= 0, "sbrk can only allocate memory, not reduce" );
   FC_ASSERT( wasm.current_memory_management != nullptr, "sbrk can only be called during the scope of wasm_interface::vm_call" );
   U32 previous_bytes_allocated = wasm.current_memory_management->sbrk(num_bytes);
   checktime(wasm.current_execution_time(), wasm_interface::get().checktime_limit);
   return previous_bytes_allocated;
}


/**
 * transaction C API implementation
 * @{
 */ 

DEFINE_INTRINSIC_FUNCTION0(env,transaction_create,transaction_create,i32) {
   auto& ptrx = wasm_interface::get().current_apply_context->create_pending_transaction();
   return ptrx.handle;
}

static void emplace_scope(const name& scope, std::vector<name>& scopes) {
   auto i = std::upper_bound( scopes.begin(), scopes.end(), scope);
   if (i == scopes.begin() || *(i - 1) != scope ) {
     scopes.insert(i, scope);
   }
}

DEFINE_INTRINSIC_FUNCTION3(env,transaction_require_scope,transaction_require_scope,none,i32,handle,i64,scope,i32,readOnly) {
   auto& ptrx = wasm_interface::get().current_apply_context->get_pending_transaction(handle);
   if(readOnly == 0) {
      emplace_scope(scope, ptrx.scope);
   } else {
      emplace_scope(scope, ptrx.read_scope);
   }

   ptrx.check_size();
}

DEFINE_INTRINSIC_FUNCTION2(env,transaction_add_message,transaction_add_message,none,i32,handle,i32,msg_handle) {
   auto apply_context  = wasm_interface::get().current_apply_context;
   auto& ptrx = apply_context->get_pending_transaction(handle);
   auto& pmsg = apply_context->get_pending_message(msg_handle);
   ptrx.messages.emplace_back(pmsg);
   ptrx.check_size();
   apply_context->release_pending_message(msg_handle);
}

DEFINE_INTRINSIC_FUNCTION1(env,transaction_send,transaction_send,none,i32,handle) {
   auto apply_context  = wasm_interface::get().current_apply_context;
   auto& ptrx = apply_context->get_pending_transaction(handle);

   EOS_ASSERT(ptrx.messages.size() > 0, tx_unknown_argument,
      "Attempting to send a transaction with no messages");

   EOS_ASSERT(false, api_not_supported,
      "transaction_send is unsupported in this release");

   apply_context->deferred_transactions.emplace_back(ptrx);
   apply_context->release_pending_transaction(handle);
}

DEFINE_INTRINSIC_FUNCTION1(env,transaction_drop,transaction_drop,none,i32,handle) {
   wasm_interface::get().current_apply_context->release_pending_transaction(handle);
}

DEFINE_INTRINSIC_FUNCTION4(env,message_create,message_create,i32,i64,code,i64,type,i32,data,i32,length) {
   auto& wasm  = wasm_interface::get();
   auto  mem   = wasm.current_memory;
   
   EOS_ASSERT( length >= 0, tx_unknown_argument,
      "Pushing a message with a negative length" );

   bytes payload;
   if (length > 0) {
      try {
         // memoryArrayPtr checks that the entire array of bytes is valid and
         // within the bounds of the memory segment so that transactions cannot pass
         // bad values in attempts to read improper memory
         const char* buffer = memoryArrayPtr<const char>( mem, uint32_t(data), uint32_t(length) );
         payload.insert(payload.end(), buffer, buffer + length);
      } catch( const Runtime::Exception& e ) {
         FC_THROW_EXCEPTION(tx_unknown_argument, "Message data is not a valid memory range");
      }
   }

   auto& pmsg = wasm.current_apply_context->create_pending_message(name(code), name(type), payload);
   return pmsg.handle;
}

DEFINE_INTRINSIC_FUNCTION3(env,message_require_permission,message_require_permission,none,i32,handle,i64,account,i64,permission) {
   auto apply_context  = wasm_interface::get().current_apply_context;
   // if this is not sent from the code account with the permission of "code" then we must
   // presently have the permission to add it, otherwise its a failure
   if (!(account == apply_context->code.value && name(permission) == name("code"))) {
      apply_context->require_authorization(name(account), name(permission));
   }
   auto& pmsg = apply_context->get_pending_message(handle);
   pmsg.authorization.emplace_back(name(account), name(permission));
}

DEFINE_INTRINSIC_FUNCTION1(env,message_send,message_send,none,i32,handle) {
   auto apply_context  = wasm_interface::get().current_apply_context;
   auto& pmsg = apply_context->get_pending_message(handle);

   apply_context->inline_messages.emplace_back(pmsg);
   apply_context->release_pending_message(handle);
}

DEFINE_INTRINSIC_FUNCTION1(env,message_drop,message_drop,none,i32,handle) {
   wasm_interface::get().current_apply_context->release_pending_message(handle);
}

/**
 * @} transaction C API implementation
 */ 



DEFINE_INTRINSIC_FUNCTION2(env,read_message,read_message,i32,i32,destptr,i32,destsize) {
   FC_ASSERT( destsize > 0 );

   wasm_interface& wasm = wasm_interface::get();
   auto  mem   = wasm.current_memory;
   char* begin = memoryArrayPtr<char>( mem, destptr, uint32_t(destsize) );

   int minlen = std::min<int>(wasm.current_validate_context->msg.data.size(), destsize);

//   wdump((destsize)(wasm.current_validate_context->msg.data.size()));
   memcpy( begin, wasm.current_validate_context->msg.data.data(), minlen );
   return minlen;
}

DEFINE_INTRINSIC_FUNCTION2(env,assert,assert,none,i32,test,i32,msg) {
   const char* m = &Runtime::memoryRef<char>( wasm_interface::get().current_memory, msg );
  std::string message( m );
  if( !test ) edump((message));
  FC_ASSERT( test, "assertion failed: ${s}", ("s",message)("ptr",msg) );
}

DEFINE_INTRINSIC_FUNCTION0(env,message_size,message_size,i32) {
   return wasm_interface::get().current_validate_context->msg.data.size();
}

DEFINE_INTRINSIC_FUNCTION1(env,malloc,malloc,i32,i32,size) {
   FC_ASSERT( size > 0 );
   int32_t& end = Runtime::memoryRef<int32_t>( Runtime::getDefaultMemory(wasm_interface::get().current_module), 0);
   int32_t old_end = end;
   end += 8*((size+7)/8);
   FC_ASSERT( end > old_end );
   return old_end;
}

DEFINE_INTRINSIC_FUNCTION1(env,printi,printi,none,i64,val) {
  std::cerr << uint64_t(val);
}
DEFINE_INTRINSIC_FUNCTION1(env,printd,printd,none,i64,val) {
  std::cerr << DOUBLE(*reinterpret_cast<double *>(&val));
}

DEFINE_INTRINSIC_FUNCTION1(env,printi128,printi128,none,i32,val) {
  auto& wasm  = wasm_interface::get();
  auto  mem   = wasm.current_memory;
  auto& value = memoryRef<unsigned __int128>( mem, val );
  fc::uint128_t v(value>>64, uint64_t(value) );
  std::cerr << fc::variant(v).get_string();
}
DEFINE_INTRINSIC_FUNCTION1(env,printn,printn,none,i64,val) {
  std::cerr << name(val).to_string();
}

DEFINE_INTRINSIC_FUNCTION1(env,prints,prints,none,i32,charptr) {
  auto& wasm  = wasm_interface::get();
  auto  mem   = wasm.current_memory;

  const char* str = &memoryRef<const char>( mem, charptr );

  std::cerr << std::string( str, strnlen(str, wasm.current_state->mem_end-charptr) );
}

DEFINE_INTRINSIC_FUNCTION2(env,prints_l,prints_l,none,i32,charptr,i32,len) {
  auto& wasm  = wasm_interface::get();
  auto  mem   = wasm.current_memory;

  const char* str = &memoryRef<const char>( mem, charptr );

  std::cerr << std::string( str, len );
}

DEFINE_INTRINSIC_FUNCTION2(env,printhex,printhex,none,i32,data,i32,datalen) {
  auto& wasm  = wasm_interface::get();
  auto  mem   = wasm.current_memory;

  char* buff = memoryArrayPtr<char>(mem, data, datalen);
  std::cerr << fc::to_hex(buff, datalen);
}


DEFINE_INTRINSIC_FUNCTION1(env,free,free,none,i32,ptr) {
}

DEFINE_INTRINSIC_FUNCTION2(env,account_balance_get,account_balance_get,i32,i32,charptr,i32,len) {
  auto& wasm  = wasm_interface::get();
  auto  mem   = wasm.current_memory;

  const uint32_t account_balance_size = sizeof(account_balance);
  FC_ASSERT( len == account_balance_size, "passed in len ${len} is not equal to the size of an account_balance struct == ${real_len}", ("len",len)("real_len",account_balance_size) );

  account_balance& total_balance = memoryRef<account_balance>( mem, charptr );

  wasm.current_apply_context->require_scope(total_balance.account);

  auto& db = wasm.current_apply_context->db;
  auto* balance        = db.find< balance_object,by_owner_name >( total_balance.account );
  auto* staked_balance = db.find<staked_balance_object,by_owner_name>( total_balance.account );

  if (balance == nullptr || staked_balance == nullptr)
     return false;

  total_balance.eos_balance          = asset(balance->balance, EOS_SYMBOL);
  total_balance.staked_balance       = asset(staked_balance->staked_balance);
  total_balance.unstaking_balance    = asset(staked_balance->unstaking_balance);
  total_balance.last_unstaking_time  = staked_balance->last_unstaking_time;

  return true;
}

   wasm_interface& wasm_interface::get() {
      static wasm_interface*  wasm = nullptr;
      if( !wasm )
      {
         wlog( "Runtime::init" );
         Runtime::init();
         wasm = new wasm_interface();
      }
      return *wasm;
   }



   struct RootResolver : Runtime::Resolver
   {
      std::map<std::string,Resolver*> moduleNameToResolverMap;

     bool resolve(const std::string& moduleName,const std::string& exportName,ObjectType type,ObjectInstance*& outObject) override
     {
         // Try to resolve an intrinsic first.
         if(IntrinsicResolver::singleton.resolve(moduleName,exportName,type,outObject)) { return true; }
         FC_ASSERT( !"unresolvable", "${module}.${export}", ("module",moduleName)("export",exportName) );
         return false;
     }
   };

   int64_t wasm_interface::current_execution_time()
   {
      return (fc::time_point::now() - checktimeStart).count();
   }


   char* wasm_interface::vm_allocate( int bytes ) {
      FunctionInstance* alloc_function = asFunctionNullable(getInstanceExport(current_module,"alloc"));
      const FunctionType* functionType = getFunctionType(alloc_function);
      FC_ASSERT( functionType->parameters.size() == 1 );
      std::vector<Value> invokeArgs(1);
      invokeArgs[0] = U32(bytes);

      checktimeStart = fc::time_point::now();

      auto result = Runtime::invokeFunction(alloc_function,invokeArgs);

      return &memoryRef<char>( current_memory, result.i32 );
   }

   U32 wasm_interface::vm_pointer_to_offset( char* ptr ) {
      return U32(ptr - &memoryRef<char>(current_memory,0));
   }

   void  wasm_interface::vm_call( const char* name ) {
   try {
      std::unique_ptr<wasm_memory> wasm_memory_mgmt;
      try {
         /*
         name += "_" + std::string( current_validate_context->msg.code ) + "_";
         name += std::string( current_validate_context->msg.type );
         */
         /// TODO: cache this somehow
         FunctionInstance* call = asFunctionNullable(getInstanceExport(current_module,name) );
         if( !call ) {
            //wlog( "unable to find call ${name}", ("name",name));
            return;
         }
         //FC_ASSERT( apply, "no entry point found for ${call}", ("call", std::string(name))  );

         FC_ASSERT( getFunctionType(call)->parameters.size() == 2 );

  //       idump((current_validate_context->msg.code)(current_validate_context->msg.type)(current_validate_context->code));
         std::vector<Value> args = { Value(uint64_t(current_validate_context->msg.code)),
                                     Value(uint64_t(current_validate_context->msg.type)) };

         auto& state = *current_state;
         char* memstart = &memoryRef<char>( current_memory, 0 );
         memset( memstart + state.mem_end, 0, ((1<<16) - state.mem_end) );
         memcpy( memstart, state.init_memory.data(), state.mem_end);

         checktimeStart = fc::time_point::now();
         wasm_memory_mgmt.reset(new wasm_memory(*this));

         Runtime::invokeFunction(call,args);
         wasm_memory_mgmt.reset();
         checktime(current_execution_time(), checktime_limit);
      } catch( const Runtime::Exception& e ) {
          edump((std::string(describeExceptionCause(e.cause))));
          edump((e.callStack));
          throw;
      }
   } FC_CAPTURE_AND_RETHROW( (name)(current_validate_context->msg.type) ) }

   void  wasm_interface::vm_apply()        { vm_call("apply" );          }

   void  wasm_interface::vm_onInit()
   { try {
      try {
         FunctionInstance* apply = asFunctionNullable(getInstanceExport(current_module,"init"));
         if( !apply ) {
            elog( "no onInit method found" );
            return; /// if not found then it is a no-op
         }

         checktimeStart = fc::time_point::now();

         const FunctionType* functionType = getFunctionType(apply);
         FC_ASSERT( functionType->parameters.size() == 0 );

         std::vector<Value> args(0);

         Runtime::invokeFunction(apply,args);
      } catch( const Runtime::Exception& e ) {
         edump((std::string(describeExceptionCause(e.cause))));
         edump((e.callStack));
         throw;
      }
   } FC_CAPTURE_AND_RETHROW() }

   void wasm_interface::validate( apply_context& c ) {
      /*
      current_validate_context       = &c;
      current_precondition_context   = nullptr;
      current_apply_context          = nullptr;

      load( c.code, c.db );
      vm_validate();
      */
   }
   void wasm_interface::precondition( apply_context& c ) {
   try {

      /*
      current_validate_context       = &c;
      current_precondition_context   = &c;

      load( c.code, c.db );
      vm_precondition();
      */

   } FC_CAPTURE_AND_RETHROW() }


   void wasm_interface::apply( apply_context& c, uint32_t execution_time, bool received_block ) {
    try {
      current_validate_context       = &c;
      current_precondition_context   = &c;
      current_apply_context          = &c;
      checktime_limit                = execution_time;

      load( c.code, c.db );
      // if this is a received_block, then ignore the table_key_types
      if (received_block)
         table_key_types = nullptr;

      auto rate_limiting = c.db.find<rate_limiting_object, by_name>(c.code);
      if (rate_limiting == nullptr)
      {
         c.mutable_db.create<rate_limiting_object>([code=c.code](rate_limiting_object& rlo) {
            rlo.name = code;
            rlo.per_code_account_db_bytes = 0;
         });
      }
      else
         table_storage = rate_limiting->per_code_account_db_bytes;

      vm_apply();

      EOS_ASSERT(table_storage <= (per_code_account_max_db_limit_mbytes * bytes_per_mbyte), tx_code_db_limit_exceeded, "Database limit exceeded for account=${name}",("name", c.code));

      rate_limiting = c.db.find<rate_limiting_object, by_name>(c.code);
      c.mutable_db.modify(*rate_limiting, [storage=this->table_storage] (rate_limiting_object& rlo) {
         rlo.per_code_account_db_bytes = storage;
      });
   } FC_CAPTURE_AND_RETHROW() }

   void wasm_interface::init( apply_context& c ) {
    try {
      current_validate_context       = &c;
      current_precondition_context   = &c;
      current_apply_context          = &c;
      checktime_limit                = CHECKTIME_LIMIT;

      load( c.code, c.db );

      auto rate_limiting = c.db.find<rate_limiting_object, by_name>(c.code);
      if (rate_limiting == nullptr)
      {
         c.mutable_db.create<rate_limiting_object>([code=c.code](rate_limiting_object& rlo) {
            rlo.name = code;
            rlo.per_code_account_db_bytes = 0;
         });
      }
      else
         table_storage = rate_limiting->per_code_account_db_bytes;

      vm_onInit();

      EOS_ASSERT(table_storage <= (per_code_account_max_db_limit_mbytes * bytes_per_mbyte), tx_code_db_limit_exceeded, "Database limit exceeded for account=${name}",("name", c.code));

      rate_limiting = c.db.find<rate_limiting_object, by_name>(c.code);
      c.mutable_db.modify(*rate_limiting, [storage=this->table_storage] (rate_limiting_object& rlo) {
         rlo.per_code_account_db_bytes = storage;
      });
   } FC_CAPTURE_AND_RETHROW() }



   void wasm_interface::load( const account_name& name, const chainbase::database& db ) {
      const auto& recipient = db.get<account_object,by_name>( name );
  //    idump(("recipient")(name(name))(recipient.code_version));

      auto& state = instances[name];
      if( state.code_version != recipient.code_version ) {
        if( state.instance ) {
           /// TODO: free existing instance and module
#warning TODO: free existing module if the code has been updated, currently leak memory
           state.instance     = nullptr;
           state.module       = nullptr;
           state.code_version = fc::sha256();
        }
        state.module = new IR::Module();
        state.table_key_types.clear();

        try
        {
//          wlog( "LOADING CODE" );
          const auto start = fc::time_point::now();
          Serialization::MemoryInputStream stream((const U8*)recipient.code.data(),recipient.code.size());
          WASM::serializeWithInjection(stream,*state.module);

          RootResolver rootResolver;
          LinkResult linkResult = linkModule(*state.module,rootResolver);
          state.instance = instantiateModule( *state.module, std::move(linkResult.resolvedImports) );
          FC_ASSERT( state.instance );
          const auto llvm_time = fc::time_point::now();

          current_memory = Runtime::getDefaultMemory(state.instance);
            
          char* memstart = &memoryRef<char>( current_memory, 0 );
         // state.init_memory.resize(1<<16); /// TODO: actually get memory size
          const auto allocated_memory = Runtime::getDefaultMemorySize(state.instance);
          for( uint64_t i = 0; i < allocated_memory; ++i )
          {
             if( memstart[i] )
             {
                state.mem_end = i+1;
             }
          }
          //ilog( "INIT MEMORY: ${size}", ("size", state.mem_end) );

          state.init_memory.resize(state.mem_end);
          memcpy( state.init_memory.data(), memstart, state.mem_end ); //state.init_memory.size() );
          //std::cerr <<"\n";
          state.code_version = recipient.code_version;
//          idump((state.code_version));
          const auto init_time = fc::time_point::now();

          types::abi abi;
          if( types::abi_serializer::to_abi(recipient.abi, abi) )
          {
             state.tables_fixed = true;
             for(auto& table : abi.tables)
             {
                const auto key_type = to_key_type(table.index_type);
                if (key_type == invalid_key_type)
                   throw Serialization::FatalSerializationException("For code \"" + (std::string)name + "\" index_type of \"" +
                                                                    table.index_type + "\" referenced but not supported");

                state.table_key_types.emplace(std::make_pair(table.table_name, key_type));
             }
          }
          ilog("wasm_interface::load times llvm:${llvm} ms, init:${init} ms, abi:${abi} ms",
               ("llvm",(llvm_time-start).count()/1000)("init",(init_time-llvm_time).count()/1000)("abi",(fc::time_point::now()-init_time).count()/1000));
        }
        catch(Serialization::FatalSerializationException exception)
        {
          std::cerr << "Error deserializing WebAssembly binary file:" << std::endl;
          std::cerr << exception.message << std::endl;
          throw;
        }
        catch(IR::ValidationException exception)
        {
          std::cerr << "Error validating WebAssembly binary file:" << std::endl;
          std::cerr << exception.message << std::endl;
          throw;
        }
        catch(std::bad_alloc)
        {
          std::cerr << "Memory allocation failed: input is likely malformed" << std::endl;
          throw;
        }
      }
      current_module      = state.instance;
      current_memory      = getDefaultMemory( current_module );
      current_state       = &state;
      table_key_types     = &state.table_key_types;
      tables_fixed        = state.tables_fixed;
      table_storage       = 0;
   }

   wasm_memory::wasm_memory(wasm_interface& interface)
   : _wasm_interface(interface)
   , _num_pages(Runtime::getMemoryNumPages(interface.current_memory))
   , _min_bytes(limit_32bit_address(_num_pages << numBytesPerPageLog2))
   {
      _wasm_interface.current_memory_management = this;
      _num_bytes = _min_bytes;
   }

   wasm_memory::~wasm_memory()
   {
      if (_num_bytes > _min_bytes)
         sbrk((I32)_min_bytes - (I32)_num_bytes);

      _wasm_interface.current_memory_management = nullptr;
   }

   U32 wasm_memory::sbrk(I32 num_bytes)
   {
      const U32 previous_num_bytes = _num_bytes;
      if(Runtime::getMemoryNumPages(_wasm_interface.current_memory) != _num_pages)
         throw eosio::chain::page_memory_error();

      // Round the absolute value of num_bytes to an alignment boundary, and ensure it won't allocate too much or too little memory.
      num_bytes = (num_bytes + 7) & ~7;
      if(num_bytes > 0 && previous_num_bytes > _max_memory - num_bytes)
         throw eosio::chain::page_memory_error();
      else if(num_bytes < 0 && previous_num_bytes < _min_bytes - num_bytes)
         throw eosio::chain::page_memory_error();

      // Update the number of bytes allocated, and compute the number of pages needed for it.
      _num_bytes += num_bytes;
      const Uptr num_desired_pages = (_num_bytes + IR::numBytesPerPage - 1) >> IR::numBytesPerPageLog2;

      // Grow or shrink the memory object to the desired number of pages.
      if(num_desired_pages > _num_pages)
         Runtime::growMemory(_wasm_interface.current_memory, num_desired_pages - _num_pages);
      else if(num_desired_pages < _num_pages)
         Runtime::shrinkMemory(_wasm_interface.current_memory, _num_pages - num_desired_pages);

      _num_pages = num_desired_pages;

      return previous_num_bytes;
   }

   U32 wasm_memory::limit_32bit_address(Uptr address)
   {
      return (U32)(address > UINT32_MAX ? UINT32_MAX : address);
   }

} }
