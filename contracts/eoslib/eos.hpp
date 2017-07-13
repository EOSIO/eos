#pragma once
#include <eoslib/types.h>

extern "C" {

/**
 * Multiplying two 128 bit numbers creates a 256 bit temporary that is then divided by
 * a 128 bit number and cast back to 128 bits posing some precision in the process.
 *
 * This method performs the following operation:
 *
 *    result = uint128( (uint256(a)*mult) / div) 
 *
 * @pre *div != 0
 * @post *result contains the result of the above math
void mult_div_i128( const uint128_t* a, const uint128_t* mult, const uint128_t* div, uint128_t* result );
 */

/**
 *  @param msg - a pointer where up to @ref len bytes of the current message will be coppied
 *  @return the number of bytes copied to msg
 */
uint32_t readMessage( void* msg, uint32_t len );
/**
 * This method is useful for dynamicly sized messages
 *
 * @return the length of the current message
 */
uint32_t messageSize();

void prints( const char* cstr );
void printi( uint64_t value );
void printi128( const uint128_t* value );
void printn( uint64_t name );

/**
 *  @return the account which specifes the code that is being run
 */
AccountName currentCode();
void  assert( uint32_t test, const char* cstr );

/**
 *  Verifies that @ref name exists in the set of notified accounts on a message. Throws if not found
 */
void        requireNotice( AccountName );
/**
 *  Verifies that @ref name exists in the set of provided auths on a message. Throws if not found
 */
void        requireAuth( AccountName name );

/**
 *  Gets the notified account at index N
 */
AccountName getNotify( int32_t index );

/**
 *  Gets the required auth at index N
 */
AccountName getAuth( int32_t index );

/**
 *  Returns the time of the last block (not the block including this message)
 */
Time  now();

/**
 * Any message handler can generate an "output" to be returned if it calls the callResult() API, if
 * callResult() is not called then result_length will be set to 0.
 *
 * @param code   - the account whose code should execute within the notify account contexts
 * @param notify - any account in the notify list must be within the current read scope
 * @param message - the message that should be delivered
 * @param result  - a place to store the result of the call
 *
 * @param canfail - if true then a new undo context is started and undon on error, else this method throws on error
 * @return 0 on success and 1 if the call aborted
 */
uint32_t    syncCall( AccountName code, AccountName* authorities, uint32_t numauths,
                                        AccountName* notify, uint32_t numnotice,
                                        const void*  message, uint32_t message_length,
                                        void* result, uint32_t* result_length, 
                                        bool canfail );

/**
 * Used to specify the return value for syncCall
 */
void        callResult( const void* data, uint32_t datalen );

/**
 *  Given the message with CODE.ACTION notifying CONTEXT we normally also allow
 *  CONTEXT to define its own method handler that can be called: eg context::apply_code_action()
 *
 *  In some cases the code::apply_code_action() may want to prevent context::apply_code_action()
 *  from being invoked.  This is necessary if the context may have incentive to block a particular
 *  contract from modifying data stored in the context/code section.  
 *
 *  For example a social media website that stores votes on accounts and an account owner interested
 *  in blocking negative votes.  
 */
void disableContextCode( uint64_t AccountName );

} /// extern C

template<typename T>
T min( const T& a, const T&b ) {
  return a < b ? a : b;
}

static constexpr char char_to_symbol( char c ) {
   if( c >= 'a' && c <= 'z' )
      return (c - 'a') + 1;
   if( c >= '1' && c <= '5' )
      return (c - '1') + 26;
   return 0;
}

static constexpr uint64_t string_to_name( const char* str ) {
   uint32_t len = 0;
   while( str[len] ) ++len;

   uint64_t value = 0;

   for( uint32_t i = 0; i <= 12 && i < len; ++i ) {
      value <<= 5;
      value |= char_to_symbol( str[ len -1 - i ] );
   }

   if( len == 13 ) {
      value <<= 4;
      value |= 0x0f & char_to_symbol( str[ 12 ] );
   }
   return value;
}
template<uint64_t I>
struct ConstName { 
   static uint64_t value() { return I; } 
   operator uint64_t()const { return I; }
};
#define NAME(X) (ConstName<string_to_name(X)>::value())
#define N(X) string_to_name(#X)

struct Name {
   Name(){}
//   Name( const char* c ) : value( string_to_name(c) ){}
   Name( uint64_t v ): value(v) {}

   operator uint64_t()const { return value; }

   friend bool operator==( const Name& a, const Name& b ) { return a.value == b.value; }
   AccountName value = 0;
};


template<typename T> struct remove_reference           { typedef T type; };
template<typename T> struct remove_reference<T&>       { typedef T type; };
template<typename T> struct remove_reference<const T&> { typedef T type; };

template<typename T>
T currentMessage() {
   T value;
   readMessage( &value, sizeof(value) );
   return value;
}

template<typename... Accounts>
void requireNotice( AccountName name, Accounts... accounts ){
   requireNotice( name );
   requireNotice( accounts... );
}


struct Ratio {
   uint64_t base  = 1;
   uint64_t quote = 1;
};
static_assert( sizeof(Ratio) == 2*sizeof(uint64_t), "unexpected padding" );


/*

template<uint64_t table, typename Record, typename Primary, typename Secondary>
struct PrimaryIndex {
   typedef table_impl<sizeof( Primary ), sizeof( Secondary )> impl;


   bool front( Record& r )const {
      return impl::front_primary( scope, code, table, &r, sizeof(Record) ) == sizeof(Record);
   }
   bool back( Record& r )const {
      return impl::back_primary( scope, code, table, &r, sizeof(Record) ) == sizeof(Record);
   }
   bool next( Record& r )const {
      return impl::next_primary( scope, code, table, &r, sizeof(Record) ) == sizeof(Record);
   }
   bool previous( Record& r )const {
      return impl::previous_primary( scope, code, table, &r, sizeof(Record) ) == sizeof(Record);
   }
   bool get( const Primary& p, Record& r )const {
      return impl::load_primary( scope, code, table, &p, &r, sizeof(Record) ) == sizeof(Record);
   }
   bool lower_bound( const Primary& p, Record& r )const {
      return impl::lower_bound_primary( scope, code, table, &p &r, sizeof(Record) ) == sizeof(Record);
   }
   bool upper_bound( const Primary& p, Record& r )const {
      return impl::upper_bound_primary( scope, code, table, &p &r, sizeof(Record) ) == sizeof(Record);
   }
   bool remove( Record& r )const {
      impl::remove( scope, table, &r );
   }

   PrimaryIndex( AccountName _scope, AccountName _code ):scope(_scope),code(_code){}

   private:
      AccountName scope; 
      AccountName code;
};


template<uint64_t scope, uint64_t code, uint64_t table, typename Record, typename Primary, typename Secondary>
struct PrimaryIndex {
   private:
   typedef table_impl<sizeof( Primary ), sizeof( Secondary )> impl;

   public:

   static bool front( Record& r ) {
      return impl::front_primary( scope, code, table, &r, sizeof(Record) ) == sizeof(Record);
   }
   static bool back( Record& r ) {
      return impl::back_primary( scope, code, table, &r, sizeof(Record) ) == sizeof(Record);
   }
   static bool next( Record& r ) {
      return impl::next_primary( scope, code, table, &r, sizeof(Record) ) == sizeof(Record);
   }
   static bool previous( Record& r ) {
      return impl::previous_primary( scope, code, table, &r, sizeof(Record) ) == sizeof(Record);
   }
   static bool get( const Primary& p, Record& r ) {
      return impl::load_primary( scope, code, table, &p, &r, sizeof(Record) ) == sizeof(Record);
   }
   static bool lower_bound( const Primary& p, Record& r ) {
      return impl::lower_bound_primary( scope, code, table, &p &r, sizeof(Record) ) == sizeof(Record);
   }
   static bool upper_bound( const Primary& p, Record& r ) {
      return impl::upper_bound_primary( scope, code, table, &p &r, sizeof(Record) ) == sizeof(Record);
   }
   static bool remove( Record& r ) {
      impl::remove( scope, table, &r );
   }
};

template<uint64_t scope, uint64_t code, uint64_t table, typename Record, typename Primary, typename Secondary>
struct SecondaryIndex {
   private:
   typedef table_impl<sizeof( Primary ), sizeof( Secondary )> impl;

   public:
   static bool front( Record& r ) {
      return impl::front_secondary( scope, code, table, &r, sizeof(Record) ) == sizeof(Record);
   }
   static bool back( Record& r ) {
      return impl::back_secondary( scope, code, table, &r, sizeof(Record) ) == sizeof(Record);
   }
   static bool next( Record& r ) {
      return impl::next_secondary( scope, code, table, &r, sizeof(Record) ) == sizeof(Record);
   }
   static bool previous( Record& r ) {
      return impl::previous_secondary( scope, code, table, &r, sizeof(Record) ) == sizeof(Record);
   }
   static bool get( const Primary& p, Record& r ) {
      return impl::load_secondary( scope, code, table, &p &r, sizeof(Record) ) == sizeof(Record);
   }
   static bool lower_bound( const Primary& p, Record& r ) {
      return impl::lower_bound_secondary( scope, code, table, &p &r, sizeof(Record) ) == sizeof(Record);
   }
   static bool upper_bound( const Primary& p, Record& r ) {
      return impl::upper_bound_secondary( scope, code, table, &p &r, sizeof(Record) ) == sizeof(Record);
   }
   static bool remove( Record& r ) {
      impl::remove( scope, table, &r );
   }
};
template<uint64_t scope, uint64_t code, uint64_t table, typename Record, typename Primary, typename Secondary>
struct SecondaryIndex<scope,code,table,Record,Primary,void> {}

template<uint64_t scope, uint64_t code, uint64_t table, typename Record, typename PrimaryType, typename SecondaryType>
struct Table {
   private:
   typedef table_impl<sizeof( PrimaryType ), sizeof( SecondaryType )> impl;

   public:
   typedef PrimaryIndex<scope,code,table,Record,PrimaryType,SecondaryType>   Primary;
   typedef SecondaryIndex<scope,code,table,Record,PrimaryType,SecondaryType> Secondary;

   static bool store( Record& r ) {
      return impl::store( scope, table, &r, sizeof(r) );
   }
   static bool remove( Record& r ) {
      return impl::remove( scope, table, &r );
   }
};

#define TABLE2(SCOPE, CODE, TABLE, TYPE, NAME, PRIMARY_NAME, PRIMARY_TYPE, SECONDARY_NAME, SECONDARY_TYPE) \
   using NAME = Table<N(SCOPE),N(CODE),N(TABLE),TYPE,PRIMARY_TYPE,SECONDARY_TYPE>; \
   typedef NAME::Primary PRIMARY_NAME; \
   typedef NAME::Secondary SECONDARY_NAME; 

#define TABLE(SCOPE, CODE, TABLE, TYPE, NAME, PRIMARY_NAME, PRIMARY_TYPE) \
   using NAME = Table<N(SCOPE),N(CODE),N(TABLE),TYPE,PRIMARY_TYPE,void>; \
   typedef NAME::Primary PRIMARY_NAME; 
*/

