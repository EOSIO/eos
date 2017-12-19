/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
extern "C" {
typedef long long    uint64_t;
typedef unsigned int uint32_t;
typedef uint64_t     AccountName;
int load( const void* keyptr, int keylen, void* valueptr, int valuelen );
void store( const void* keyptr, int keylen, const void* valueptr, int valuelen );
int read_message( void* dest, int destsize );
int remove( const void* key, int keyLength );
void printi( uint64_t );
void print( const char* str );
void assert( int test, const char* message );
void* memcpy( void* dest, const void* src, uint32_t size );
uint64_t name_to_int64( const char* name );
/*
void* malloc( unsigned int size ) {
    static char dynamic_memory[1024*8];
    static int  start = 0;
    int old_start = start;
    start +=  8*((size+7)/8);
    assert( start < sizeof(dynamic_memory), "out of memory" );
    return &dynamic_memory[old_start];
}
*/
}


template<typename Key, typename Value>
int load( const Key& key,  Value& v ) { return load( &key, sizeof(Key), &v, sizeof(Value) ); }
template<typename Key, typename Value>
void store( const Key& key, const Value& v ) { store( &key, sizeof(key), &v, sizeof(v) ); }
template<typename Key>
void remove( const Key& key ) { remove( &key, sizeof(key) );  }
template<typename Message>
void read_message( Message& m ) { read_message( &m, sizeof(Message) ); }
/// END BUILT IN LIBRARY.... everything below this is "user contract"


extern "C" {
struct transfer {
  uint64_t    from;
  uint64_t    to;
  uint64_t    amount;
  char        memo[];
};

static_assert( sizeof(transfer) == 3*sizeof(uint64_t), "unexpected padding" );

struct Balance {
  uint64_t    balance;
};

void init() {
  static Balance initial = { uint64_t(10)*1000*1000ll*1000ll };
  static AccountName simplecoin;
  simplecoin = name_to_int64( "simplecoin" );
  print( "on_init called with "); printi( initial.balance ); print( "\n");
  store( simplecoin, initial ); 
}

void apply_simplecoin_transfer() {
   static transfer message;
   static Balance from_balance;
   static Balance to_balance;
   to_balance.balance = 0;

   read_message( message  );
   load( message.from, from_balance );
   load( message.to, to_balance );

   assert( from_balance.balance >= message.amount, "insufficient funds" );
   
   from_balance.balance -= message.amount;
   to_balance.balance   += message.amount;
   
   if( from_balance.balance )
      store( message.from, from_balance );
   else
      remove( message.from );

   store( message.to, to_balance );
}
} // extern "C" 
