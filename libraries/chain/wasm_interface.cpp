#include <boost/function.hpp>
#include <boost/multiprecision/cpp_bin_float.hpp>
#include <eos/chain/wasm_interface.hpp>
#include <eos/chain/chain_controller.hpp>
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
#include <chrono>

namespace eos { namespace chain {
   using namespace IR;
   using namespace Runtime;
   typedef boost::multiprecision::cpp_bin_float_50 DOUBLE;

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

   wasm_interface::wasm_interface() {
   }

#ifdef NDEBUG
   const int CHECKTIME_LIMIT = 3000;
#else
   const int CHECKTIME_LIMIT = 18000;
#endif

   void checktime(int64_t duration)
   {
      if (duration > CHECKTIME_LIMIT) {
         wlog("checktime called ${d}", ("d", duration));
         throw checktime_exceeded();
      }
   }

DEFINE_INTRINSIC_FUNCTION0(env,checktime,checktime,none) {
   checktime(wasm_interface::get().current_execution_time());
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


#define READ_RECORD(READFUNC, INDEX, SCOPE) \
   auto lambda = [&](apply_context* ctx, INDEX::value_type::key_type* keys, char *data, uint32_t datalen) -> int32_t { \
      auto res = ctx->READFUNC<INDEX, SCOPE>( Name(scope), Name(code), Name(table), keys, data, datalen); \
      if (res >= 0) res += INDEX::value_type::number_of_keys*sizeof(INDEX::value_type::key_type); \
      return res; \
   }; \
   return validate<decltype(lambda), INDEX::value_type::key_type, INDEX::value_type::number_of_keys>(valueptr, valuelen, lambda);

#define UPDATE_RECORD(UPDATEFUNC, INDEX, DATASIZE) \
   auto lambda = [&](apply_context* ctx, INDEX::value_type::key_type* keys, char *data, uint32_t datalen) -> int32_t { \
      return ctx->UPDATEFUNC<INDEX::value_type>( Name(scope), Name(ctx->code.value), Name(table), keys, data, datalen); \
   }; \
   return validate<decltype(lambda), INDEX::value_type::key_type, INDEX::value_type::number_of_keys>(valueptr, DATASIZE, lambda);

#define DEFINE_RECORD_UPDATE_FUNCTIONS(OBJTYPE, INDEX) \
   DEFINE_INTRINSIC_FUNCTION4(env,store_##OBJTYPE,store_##OBJTYPE,i32,i64,scope,i64,table,i32,valueptr,i32,valuelen) { \
      UPDATE_RECORD(store_record, INDEX, valuelen); \
   } \
   DEFINE_INTRINSIC_FUNCTION4(env,update_##OBJTYPE,update_##OBJTYPE,i32,i64,scope,i64,table,i32,valueptr,i32,valuelen) { \
      UPDATE_RECORD(update_record, INDEX, valuelen); \
   } \
   DEFINE_INTRINSIC_FUNCTION3(env,remove_##OBJTYPE,remove_##OBJTYPE,i32,i64,scope,i64,table,i32,valueptr) { \
      UPDATE_RECORD(remove_record, INDEX, sizeof(typename INDEX::value_type::key_type)*INDEX::value_type::number_of_keys); \
   }

#define DEFINE_RECORD_READ_FUNCTIONS(OBJTYPE, FUNCPREFIX, INDEX, SCOPE) \
   DEFINE_INTRINSIC_FUNCTION5(env,load_##FUNCPREFIX##OBJTYPE,load_##FUNCPREFIX##OBJTYPE,i32,i64,scope,i64,code,i64,table,i32,valueptr,i32,valuelen) { \
      READ_RECORD(load_record, INDEX, SCOPE); \
   } \
   DEFINE_INTRINSIC_FUNCTION5(env,front_##FUNCPREFIX##OBJTYPE,front_##FUNCPREFIX##OBJTYPE,i32,i64,scope,i64,code,i64,table,i32,valueptr,i32,valuelen) { \
      READ_RECORD(front_record, INDEX, SCOPE); \
   } \
   DEFINE_INTRINSIC_FUNCTION5(env,back_##FUNCPREFIX##OBJTYPE,back_##FUNCPREFIX##OBJTYPE,i32,i64,scope,i64,code,i64,table,i32,valueptr,i32,valuelen) { \
      READ_RECORD(back_record, INDEX, SCOPE); \
   } \
   DEFINE_INTRINSIC_FUNCTION5(env,next_##FUNCPREFIX##OBJTYPE,next_##FUNCPREFIX##OBJTYPE,i32,i64,scope,i64,code,i64,table,i32,valueptr,i32,valuelen) { \
      READ_RECORD(next_record, INDEX, SCOPE); \
   } \
   DEFINE_INTRINSIC_FUNCTION5(env,previous_##FUNCPREFIX##OBJTYPE,previous_##FUNCPREFIX##OBJTYPE,i32,i64,scope,i64,code,i64,table,i32,valueptr,i32,valuelen) { \
      READ_RECORD(previous_record, INDEX, SCOPE); \
   } \
   DEFINE_INTRINSIC_FUNCTION5(env,lower_bound_##FUNCPREFIX##OBJTYPE,lower_bound_##FUNCPREFIX##OBJTYPE,i32,i64,scope,i64,code,i64,table,i32,valueptr,i32,valuelen) { \
      READ_RECORD(lower_bound_record, INDEX, SCOPE); \
   } \
   DEFINE_INTRINSIC_FUNCTION5(env,upper_bound_##FUNCPREFIX##OBJTYPE,upper_bound_##FUNCPREFIX##OBJTYPE,i32,i64,scope,i64,code,i64,table,i32,valueptr,i32,valuelen) { \
      READ_RECORD(upper_bound_record, INDEX, SCOPE); \
   }

DEFINE_RECORD_UPDATE_FUNCTIONS(i64, key_value_index);
DEFINE_RECORD_READ_FUNCTIONS(i64,,key_value_index, by_scope_primary);
      
DEFINE_RECORD_UPDATE_FUNCTIONS(i128i128, key128x128_value_index);
DEFINE_RECORD_READ_FUNCTIONS(i128i128, primary_,   key128x128_value_index, by_scope_primary);
DEFINE_RECORD_READ_FUNCTIONS(i128i128, secondary_, key128x128_value_index, by_scope_secondary);

DEFINE_RECORD_UPDATE_FUNCTIONS(i64i64i64, key64x64x64_value_index);
DEFINE_RECORD_READ_FUNCTIONS(i64i64i64, primary_,   key64x64x64_value_index, by_scope_primary);
DEFINE_RECORD_READ_FUNCTIONS(i64i64i64, secondary_, key64x64x64_value_index, by_scope_secondary);
DEFINE_RECORD_READ_FUNCTIONS(i64i64i64, tertiary_,  key64x64x64_value_index, by_scope_tertiary);


#define UPDATE_RECORD_STR(FUNCTION) \
  auto lambda = [&](apply_context* ctx, std::string* keys, char *data, uint32_t datalen) -> int32_t { \
    return ctx->FUNCTION<keystr_value_object>( Name(scope), Name(ctx->code.value), Name(table), keys, data, datalen); \
  }; \
  return validate_str<decltype(lambda)>(keyptr, keylen, valueptr, valuelen, lambda);

#define READ_RECORD_STR(FUNCTION) \
  auto lambda = [&](apply_context* ctx, std::string* keys, char *data, uint32_t datalen) -> int32_t { \
    auto res = ctx->FUNCTION<keystr_value_index, by_scope_primary>( Name(scope), Name(code), Name(table), keys, data, datalen); \
    return res; \
  }; \
  return validate_str<decltype(lambda)>(keyptr, keylen, valueptr, valuelen, lambda);

DEFINE_INTRINSIC_FUNCTION6(env,store_str,store_str,i32,i64,scope,i64,table,i32,keyptr,i32,keylen,i32,valueptr,i32,valuelen) {
  UPDATE_RECORD_STR(store_record)
}
DEFINE_INTRINSIC_FUNCTION6(env,update_str,update_str,i32,i64,scope,i64,table,i32,keyptr,i32,keylen,i32,valueptr,i32,valuelen) {
  UPDATE_RECORD_STR(update_record)
}
DEFINE_INTRINSIC_FUNCTION4(env,remove_str,remove_str,i32,i64,scope,i64,table,i32,keyptr,i32,keylen) {
  int32_t valueptr=0, valuelen=0;
  UPDATE_RECORD_STR(remove_record)
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

DEFINE_INTRINSIC_FUNCTION2(env,getActiveProducers,getActiveProducers,none,i32,producers,i32,datalen) {
   auto& wasm    = wasm_interface::get();
   auto  mem     = wasm.current_memory;
   types::AccountName* dst = memoryArrayPtr<types::AccountName>( mem, producers, datalen );
   return wasm_interface::get().current_validate_context->get_active_producers(dst, datalen);
}

DEFINE_INTRINSIC_FUNCTION0(env,now,now,i32) {
   return wasm_interface::get().current_validate_context->controller.head_block_time().sec_since_epoch();
}

DEFINE_INTRINSIC_FUNCTION0(env,currentCode,currentCode,i64) {
   auto& wasm  = wasm_interface::get();
   return wasm.current_validate_context->code.value;
}

DEFINE_INTRINSIC_FUNCTION1(env,requireAuth,requireAuth,none,i64,account) {
   wasm_interface::get().current_validate_context->require_authorization( Name(account) );
}

DEFINE_INTRINSIC_FUNCTION1(env,requireNotice,requireNotice,none,i64,account) {
   wasm_interface::get().current_apply_context->require_recipient( account );
}

DEFINE_INTRINSIC_FUNCTION1(env,requireScope,requireScope,none,i64,scope) {
   wasm_interface::get().current_validate_context->require_scope( scope );
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
   checktime(wasm.current_execution_time());
   return previous_bytes_allocated;
}


/**
 * Transaction C API implementation
 * @{
 */ 

DEFINE_INTRINSIC_FUNCTION0(env,transactionCreate,transactionCreate,i32) {
   auto& ptrx = wasm_interface::get().current_apply_context->create_pending_transaction();
   return ptrx.handle;
}

static void emplace_scope(const Name& scope, std::vector<Name>& scopes) {
   auto i = std::upper_bound( scopes.begin(), scopes.end(), scope);
   if (i == scopes.begin() || *(i - 1) != scope ) {
     scopes.insert(i, scope);
   }
}

DEFINE_INTRINSIC_FUNCTION3(env,transactionRequireScope,transactionRequireScope,none,i32,handle,i64,scope,i32,readOnly) {
   auto& ptrx = wasm_interface::get().current_apply_context->get_pending_transaction(handle);
   if(readOnly == 0) {
      emplace_scope(scope, ptrx.scope);
   } else {
      emplace_scope(scope, ptrx.readscope);
   }

   ptrx.check_size();
}

DEFINE_INTRINSIC_FUNCTION2(env,transactionAddMessage,transactionAddMessage,none,i32,handle,i32,msg_handle) {
   auto apply_context  = wasm_interface::get().current_apply_context;
   auto& ptrx = apply_context->get_pending_transaction(handle);
   auto& pmsg = apply_context->get_pending_message(msg_handle);
   ptrx.messages.emplace_back(pmsg);
   ptrx.check_size();
   apply_context->release_pending_message(msg_handle);
}

DEFINE_INTRINSIC_FUNCTION1(env,transactionSend,transactionSend,none,i32,handle) {
   auto apply_context  = wasm_interface::get().current_apply_context;
   auto& ptrx = apply_context->get_pending_transaction(handle);

   EOS_ASSERT(ptrx.messages.size() > 0, tx_unknown_argument,
      "Attempting to send a transaction with no messages");

   apply_context->deferred_transactions.emplace_back(ptrx);
   apply_context->release_pending_transaction(handle);
}

DEFINE_INTRINSIC_FUNCTION1(env,transactionDrop,transactionDrop,none,i32,handle) {
   wasm_interface::get().current_apply_context->release_pending_transaction(handle);
}

DEFINE_INTRINSIC_FUNCTION4(env,messageCreate,messageCreate,i32,i64,code,i64,type,i32,data,i32,length) {
   auto& wasm  = wasm_interface::get();
   auto  mem   = wasm.current_memory;
   
   EOS_ASSERT( length >= 0, tx_unknown_argument,
      "Pushing a message with a negative length" );

   Bytes payload;
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

   auto& pmsg = wasm.current_apply_context->create_pending_message(Name(code), Name(type), payload);
   return pmsg.handle;
}

DEFINE_INTRINSIC_FUNCTION3(env,messageRequirePermission,messageRequirePermission,none,i32,handle,i64,account,i64,permission) {
   auto apply_context  = wasm_interface::get().current_apply_context;
   // if this is not sent from the code account with the permission of "code" then we must
   // presently have the permission to add it, otherwise its a failure
   if (!(account == apply_context->code.value && Name(permission) == Name("code"))) {
      apply_context->require_authorization(Name(account), Name(permission));
   }
   auto& pmsg = apply_context->get_pending_message(handle);
   pmsg.authorization.emplace_back(Name(account), Name(permission));
}

DEFINE_INTRINSIC_FUNCTION1(env,messageSend,messageSend,none,i32,handle) {
   auto apply_context  = wasm_interface::get().current_apply_context;
   auto& pmsg = apply_context->get_pending_message(handle);

   apply_context->inline_messages.emplace_back(pmsg);
   apply_context->release_pending_message(handle);
}

DEFINE_INTRINSIC_FUNCTION1(env,messageDrop,messageDrop,none,i32,handle) {
   wasm_interface::get().current_apply_context->release_pending_message(handle);
}

/**
 * @} Transaction C API implementation
 */ 



DEFINE_INTRINSIC_FUNCTION2(env,readMessage,readMessage,i32,i32,destptr,i32,destsize) {
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

DEFINE_INTRINSIC_FUNCTION0(env,messageSize,messageSize,i32) {
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
  std::cerr << Name(val).toString();
}

DEFINE_INTRINSIC_FUNCTION1(env,prints,prints,none,i32,charptr) {
  auto& wasm  = wasm_interface::get();
  auto  mem   = wasm.current_memory;

  const char* str = &memoryRef<const char>( mem, charptr );

  std::cerr << std::string( str, strnlen(str, wasm.current_state->mem_end-charptr) );
}

DEFINE_INTRINSIC_FUNCTION2(env,printhex,printhex,none,i32,data,i32,datalen) {
  auto& wasm  = wasm_interface::get();
  auto  mem   = wasm.current_memory;
  
  char* buff = memoryArrayPtr<char>(mem, data, datalen);
  std::cerr << fc::to_hex(buff, datalen) << std::endl;
}


DEFINE_INTRINSIC_FUNCTION1(env,free,free,none,i32,ptr) {
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
         checktime(current_execution_time());
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


   void wasm_interface::apply( apply_context& c ) {
    try {
      current_validate_context       = &c;
      current_precondition_context   = &c;
      current_apply_context          = &c;

      load( c.code, c.db );
      vm_apply();

   } FC_CAPTURE_AND_RETHROW() }

   void wasm_interface::init( apply_context& c ) {
    try {
      current_validate_context       = &c;
      current_precondition_context   = &c;
      current_apply_context          = &c;

      load( c.code, c.db );
      vm_onInit();

   } FC_CAPTURE_AND_RETHROW() }



   void wasm_interface::load( const AccountName& name, const chainbase::database& db ) {
      const auto& recipient = db.get<account_object,by_name>( name );
  //    idump(("recipient")(Name(name))(recipient.code_version));

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

        try
        {
//          wlog( "LOADING CODE" );
 //         auto start = fc::time_point::now();
          Serialization::MemoryInputStream stream((const U8*)recipient.code.data(),recipient.code.size());
          WASM::serializeWithInjection(stream,*state.module);

          RootResolver rootResolver;
          LinkResult linkResult = linkModule(*state.module,rootResolver);
          state.instance = instantiateModule( *state.module, std::move(linkResult.resolvedImports) );
          FC_ASSERT( state.instance );
  //        auto end = fc::time_point::now();
 //         idump(( (end-start).count()/1000000.0) );

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
      current_module = state.instance;
      current_memory = getDefaultMemory( current_module );
      current_state  = &state;
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
         throw eos::chain::page_memory_error();

      // Round the absolute value of num_bytes to an alignment boundary, and ensure it won't allocate too much or too little memory.
      num_bytes = (num_bytes + 7) & ~7;
      if(num_bytes > 0 && previous_num_bytes > _max_memory - num_bytes)
         throw eos::chain::page_memory_error();
      else if(num_bytes < 0 && previous_num_bytes < _min_bytes - num_bytes)
         throw eos::chain::page_memory_error();

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
