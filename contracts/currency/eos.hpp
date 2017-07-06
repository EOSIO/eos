extern "C" {

typedef long long            int64_t;
typedef unsigned long long   uint64_t;
typedef unsigned long        uint32_t;
typedef long                 int32_t;

typedef uint64_t AccountName;
typedef uint64_t TableName;

/**
 *  @param msg - a pointer where up to @ref len bytes of the current message will be coppied
 *  @return the number of bytes copied to msg
 */
extern uint32_t readMessage( void* msg, uint32_t len );
/**
 * This method is useful for dynamicly sized messages
 *
 * @return the length of the current message
 */
extern uint32_t messageSize();

/**
 * @param scope - the account scope that will be read, must exist in the transaction scopes list
 * @param table - the ID/name of the table within the current scope/code context to modify
 * @param key   - an key that can be used to lookup data in the table
 *
 * @return a unique ID assigned to this table record separate from the key, used for iterator access
 */
int32_t store_i64( AccountName scope, TableName table, uint64_t key, const void* data, uint32_t datalen );

/**
 *  @param scope - the account scope that will be read, must exist in the transaction scopes list
 *  @param code  - identifies the code that controls write-access to the data
 *  @param table - the ID/name of the table within the scope/code context to query
 *  @param key   - an key that can be used to lookup data in the table
 *  @param data  - location to copy the data stored at key
 *  @param datalen - the maximum length of data to read
 *
 *  @return the number of bytes read or -1 if key was not found
 */
int32_t load_i64( AccountName scope, AccountName code, TableName table, uint64_t key, void* data, uint32_t datalen );
int32_t remove_i64( AccountName scope, TableName table, uint64_t key );

/**
 *  @return the current account scope within which the currentCode() is being applied.
 */
AccountName currentContext();

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
uint32_t now();

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

struct Name {
   Name(){}
//   Name( const char* c ) : value( string_to_name(c) ){}
   Name( uint64_t v ): value(v) {}

   operator uint64_t()const { return value; }

   friend bool operator==( const Name& a, const Name& b ) { return a.value == b.value; }
   AccountName value = 0;
};

struct Db
{
   template<typename T>
   static bool test( Name key, const T& value ) {
      return T::tableId();
   }

   template<typename T>
   static bool get( Name key, T& value ){
      return get( currentCode(), key, value );
   }

   template<typename T>
   static bool get( Name scope, Name key, T& value ){
      return get( scope, currentCode(), key, value );
   }

   template<typename T>
   static bool get( Name scope, Name code, Name key, T& result ) {
      auto read = load_i64( scope.value, code.value, T::tableId().value, key.value, &result, sizeof(result) );
      return read > 0;
   }

   template<typename T>
   static int32_t store( Name key, const T& value ) {
      return store( currentCode(), key, value );
   }

   template<typename T>
   static int32_t store( Name scope, Name key, const T& value ) {
      return store_i64( scope, T::tableId(), key, &value, sizeof(value) );
   }

   template<typename T>
   static bool remove( Name scope, Name key ) {
      return remove_i64( scope, T::tableId(), key );
   }
   template<typename T>
   static bool remove( Name key ) {
      return remove<T>( currentCode(), key );
   }
};

template<typename T>
T currentMessage() {
   static T value;
   readMessage( &value, sizeof(value) );
   return value;
}

template<typename... Accounts>
void requireNotice( AccountName name, Accounts... accounts ){
   requireNotice( name );
   requireNotice( accounts... );
}

