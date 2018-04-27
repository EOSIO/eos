/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/action.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/transaction.hpp>

#include "test_api.hpp"

#pragma pack(push, 1)
template <uint64_t ACCOUNT, uint64_t NAME>
struct test_action_action {
   static account_name get_account() {
      return account_name(ACCOUNT);
   }

   static action_name get_name() {
      return action_name(NAME);
   }

   eosio::vector<char> data;

   template <typename DataStream>
   friend DataStream& operator << ( DataStream& ds, const test_action_action& a ) {
      for ( auto c : a.data )
         ds << c;
      return ds;
   }
  /*
   template <typename DataStream>
   friend DataStream& operator >> ( DataStream& ds, test_action_action& a ) {
      return ds;
   }
   */
};


template <uint64_t ACCOUNT, uint64_t NAME>
struct test_dummy_action {
   static account_name get_account() {
      return account_name(ACCOUNT);
   }

   static action_name get_name() {
      return action_name(NAME);
   }
   char a;
   unsigned long long b;
   int32_t c;

   template <typename DataStream>
   friend DataStream& operator << ( DataStream& ds, const test_dummy_action& da ) {
      ds << da.a;
      ds << da.b;
      ds << da.c;
      return ds;
   }

   template <typename DataStream>
   friend DataStream& operator >> ( DataStream& ds, test_dummy_action& da ) {
      ds >> da.a;
      ds >> da.b;
      ds >> da.c;
      return ds;
   }
};
#pragma pack(pop)

void copy_data(char* data, size_t data_len, eosio::vector<char>& data_out) {
   for (unsigned int i=0; i < data_len; i++)
      data_out.push_back(data[i]);
}

void test_transaction::send_action() {
   using namespace eosio;
   test_dummy_action<N(testapi), WASM_TEST_ACTION("test_action", "read_action_normal")> test_action = {DUMMY_ACTION_DEFAULT_A, DUMMY_ACTION_DEFAULT_B, DUMMY_ACTION_DEFAULT_C};
   action act(eosio::vector<permission_level>{{N(testapi), N(active)}}, test_action);
   act.send();
}

void test_transaction::send_action_empty() {
   using namespace eosio;
   test_action_action<N(testapi), WASM_TEST_ACTION("test_action", "assert_true")> test_action;

   action act(eosio::vector<permission_level>{{N(testapi), N(active)}}, test_action);

   act.send();
}

/**
 * cause failure due to a large action payload
 */
void test_transaction::send_action_large() {
   using namespace eosio;
   static char large_message[8 * 1024];
   test_action_action<N(testapi), WASM_TEST_ACTION("test_action", "read_action_normal")> test_action;
   copy_data(large_message, 8*1024, test_action.data);
   action act(vector<permission_level>{{N(testapi), N(active)}}, test_action);
   act.send();
   eosio_assert(false, "send_message_large() should've thrown an error");
}

/**
 * cause failure due recursive loop
 */
void test_transaction::send_action_recurse() {
   using namespace eosio;
   char buffer[1024];
   read_action_data(buffer, 1024);

   test_action_action<N(testapi), WASM_TEST_ACTION("test_transaction", "send_action_recurse")> test_action;
   copy_data(buffer, 1024, test_action.data);
   action act(vector<permission_level>{{N(testapi), N(active)}}, test_action);

   act.send();
}

/**
 * cause failure due to inline TX failure
 */
void test_transaction::send_action_inline_fail() {
   using namespace eosio;
   test_action_action<N(testapi), WASM_TEST_ACTION("test_action", "assert_false")> test_action;

   action act(vector<permission_level>{{N(testapi), N(active)}}, test_action);

   act.send();
}

void test_transaction::test_tapos_block_prefix() {
   using namespace eosio;
   int tbp;
   read_action_data( (char*)&tbp, sizeof(int) );
   eosio_assert( tbp == tapos_block_prefix(), "tapos_block_prefix does not match" );
}

void test_transaction::test_tapos_block_num() {
   using namespace eosio;
   int tbn;
   read_action_data( (char*)&tbn, sizeof(int) );
   eosio_assert( tbn == tapos_block_num(), "tapos_block_num does not match" );
}


void test_transaction::test_read_transaction() {
   using namespace eosio;
   checksum256 h;
   auto size = transaction_size();
   char buf[size];
   uint32_t read = read_transaction( buf, size );
   eosio_assert( size == read, "read_transaction failed");
   sha256(buf, read, &h);
   printhex( &h, sizeof(h) );
}

void test_transaction::test_transaction_size() {
   using namespace eosio;
   uint32_t trans_size = 0;
   read_action_data( (char*)&trans_size, sizeof(uint32_t) );
   print( "size: ", transaction_size() );
   eosio_assert( trans_size == transaction_size(), "transaction size does not match" );
}

void test_transaction::send_transaction(uint64_t receiver, uint64_t, uint64_t) {
   using namespace eosio;
   dummy_action payload = {DUMMY_ACTION_DEFAULT_A, DUMMY_ACTION_DEFAULT_B, DUMMY_ACTION_DEFAULT_C};

   test_action_action<N(testapi), WASM_TEST_ACTION("test_action", "read_action_normal")> test_action;
   copy_data((char*)&payload, sizeof(dummy_action), test_action.data);

   auto trx = transaction();
   trx.actions.emplace_back(vector<permission_level>{{N(testapi), N(active)}}, test_action);
   trx.send(0, receiver);
}

void test_transaction::send_action_sender(uint64_t receiver, uint64_t, uint64_t) {
   using namespace eosio;
   account_name cur_send;
   read_action_data( &cur_send, sizeof(account_name) );
   test_action_action<N(testapi), WASM_TEST_ACTION("test_action", "test_current_sender")> test_action;
   copy_data((char*)&cur_send, sizeof(account_name), test_action.data);

   auto trx = transaction();
   trx.actions.emplace_back(vector<permission_level>{{N(testapi), N(active)}}, test_action);
   trx.send(0, receiver);
}

void test_transaction::send_transaction_empty(uint64_t receiver, uint64_t, uint64_t) {
   using namespace eosio;
   auto trx = transaction();
   trx.send(0, receiver);

   eosio_assert(false, "send_transaction_empty() should've thrown an error");
}

void test_transaction::send_transaction_trigger_error_handler(uint64_t receiver, uint64_t, uint64_t) {
   using namespace eosio;
   auto trx = transaction();
   test_action_action<N(testapi), WASM_TEST_ACTION("test_action", "assert_false")> test_action;
   trx.actions.emplace_back(vector<permission_level>{{N(testapi), N(active)}}, test_action);
   trx.send(0, receiver);
}

void test_transaction::assert_false_error_handler(const eosio::deferred_transaction& dtrx) {
   auto onerror_action = eosio::get_action(1, 0);
   eosio_assert( onerror_action.authorization.at(0).actor == dtrx.actions.at(0).account,
                "authorizer of onerror action does not match receiver of original action in the deferred transaction" );
}

/**
 * cause failure due to a large transaction size
 */
void test_transaction::send_transaction_large(uint64_t receiver, uint64_t, uint64_t) {
   using namespace eosio;
   auto trx = transaction();
   for (int i = 0; i < 32; i ++) {
      char large_message[1024];
      test_action_action<N(testapi), WASM_TEST_ACTION("test_action", "read_action_normal")> test_action;
      copy_data(large_message, 1024, test_action.data);
      trx.actions.emplace_back(vector<permission_level>{{N(testapi), N(active)}}, test_action);
   }

   trx.send(0, receiver);

   eosio_assert(false, "send_transaction_large() should've thrown an error");
}

void test_transaction::send_transaction_expiring_late(uint64_t receiver, uint64_t, uint64_t) {
   using namespace eosio;
   account_name cur_send;
   read_action_data( &cur_send, sizeof(account_name) );
   test_action_action<N(testapi), WASM_TEST_ACTION("test_action", "test_current_sender")> test_action;
   copy_data((char*)&cur_send, sizeof(account_name), test_action.data);

   auto trx = transaction(now() + 60*60*24*365);
   trx.actions.emplace_back(vector<permission_level>{{N(testapi), N(active)}}, test_action);
   trx.send(0, receiver);

   eosio_assert(false, "send_transaction_expiring_late() should've thrown an error");
}

/**
 * deferred transaction
 */
void test_transaction::deferred_print() {
   eosio::print("deferred executed\n");
}

void test_transaction::send_deferred_transaction(uint64_t receiver, uint64_t, uint64_t) {
   using namespace eosio;
   auto trx = transaction();
   test_action_action<N(testapi), WASM_TEST_ACTION("test_transaction", "deferred_print")> test_action;
   trx.actions.emplace_back(vector<permission_level>{{N(testapi), N(active)}}, test_action);
   trx.delay_sec = 2;
   trx.send( 0xffffffffffffffff, receiver );
}

void test_transaction::cancel_deferred_transaction() {
   using namespace eosio;
   cancel_deferred( 0xffffffffffffffff ); //use the same id (0) as in send_deferred_transaction
}

void test_transaction::send_cf_action() {
   using namespace eosio;
   test_action_action<N(dummy), N(event1)> cfa;
   action act(cfa);
   act.send_context_free();
}

void test_transaction::send_cf_action_fail() {
   using namespace eosio;
   test_action_action<N(dummy), N(event1)> cfa;
   action act(vector<permission_level>{{N(dummy), N(active)}}, cfa);
   act.send_context_free();
   eosio_assert(false, "send_cfa_action_fail() should've thrown an error");
}

void test_transaction::read_inline_action() {
   using namespace eosio;
   using dummy_act_t = test_dummy_action<N(testapi), WASM_TEST_ACTION("test_action","assert_true")>;

   char buffer[64];
   auto res = get_action( 3, 0, buffer, 64);
   eosio_assert(res == -1, "get_action error 0");

   auto dummy_act = dummy_act_t{1, 2, 3};

   action act(vector<permission_level>{{N(testapi), N(active)}}, dummy_act);
   act.send();

   res = get_action( 3, 0, buffer, 64);
   eosio_assert(res != -1, "get_action error");

   action tmp;
   datastream<char *> ds(buffer, (size_t)res);
   ds >> tmp.account;
   ds >> tmp.name;
   ds >> tmp.authorization;
   ds >> tmp.data;

   auto dres = tmp.data_as<dummy_act_t>();
   eosio_assert(dres.a == 1 && dres.b == 2 && dres.c == 3, "data_as error");

   res = get_action( 3, 1, buffer, 64);
   eosio_assert(res == -1, "get_action error -1");
}

void test_transaction::read_inline_cf_action() {
   using namespace eosio;
   using dummy_act_t = test_dummy_action<N(testapi), WASM_TEST_ACTION("test_action","assert_true_cf")>;

   char buffer[64];
   auto res = get_action( 2, 0, buffer, 64);
   eosio_assert(res == -1, "get_action error 0");

   auto dummy_act = dummy_act_t{1, 2, 3};

   action act(dummy_act);
   act.send_context_free();

   res = get_action( 2, 0, buffer, 64);
   eosio_assert(res != -1, "get_action error");

   action tmp;
   datastream<char *> ds(buffer, (size_t)res);
   ds >> tmp.account;
   ds >> tmp.name;
   ds >> tmp.authorization;
   ds >> tmp.data;

   auto dres = tmp.data_as<dummy_act_t>();
   eosio_assert(dres.a == 1 && dres.b == 2 && dres.c == 3, "data_as error");

   res = get_action( 2, 1, buffer, 64);
   eosio_assert(res == -1, "get_action error -1");
}
