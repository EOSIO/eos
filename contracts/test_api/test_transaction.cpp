/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/transaction.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/crypto.h>

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
   test_dummy_action<N(testapi), WASM_TEST_ACTION("test_action", "read_action_normal")> test_action = {DUMMY_ACTION_DEFAULT_A, DUMMY_ACTION_DEFAULT_B, DUMMY_ACTION_DEFAULT_C};
   action act(eosio::vector<permission_level>{{N(testapi), N(active)}}, test_action);
   act.send();
}

void test_transaction::send_action_empty() {
   test_action_action<N(testapi), WASM_TEST_ACTION("test_action", "assert_true")> test_action;

   action act(eosio::vector<permission_level>{{N(testapi), N(active)}}, test_action);

   act.send();
}

/**
 * cause failure due to a large action payload
 */
void test_transaction::send_action_large() {
   char large_message[8 * 1024];
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
   test_action_action<N(testapi), WASM_TEST_ACTION("test_action", "assert_false")> test_action;

   action act(vector<permission_level>{{N(testapi), N(active)}}, test_action);

   act.send();
}

void test_transaction::test_tapos_block_prefix() {
   int tbp;
   read_action_data( (char*)&tbp, sizeof(int) );
   eosio_assert( tbp == tapos_block_prefix(), "tapos_block_prefix does not match" );
}

void test_transaction::test_tapos_block_num() {
   int tbn;
   read_action_data( (char*)&tbn, sizeof(int) );
   eosio_assert( tbn == tapos_block_num(), "tapos_block_num does not match" );
}


void test_transaction::test_read_transaction() {
   checksum256 h;
   transaction t;
   char* p = (char*)&t;
   uint32_t read = read_transaction( (char*)&t, sizeof(t) );
   sha256(p, read, &h);
   printhex( &h, sizeof(h) );
}

void test_transaction::test_transaction_size() {
   uint32_t trans_size = 0;
   read_action_data( (char*)&trans_size, sizeof(uint32_t) );
   eosio_assert( trans_size == transaction_size(), "transaction size does not match" );
}

void test_transaction::send_transaction() {
   dummy_action payload = {DUMMY_ACTION_DEFAULT_A, DUMMY_ACTION_DEFAULT_B, DUMMY_ACTION_DEFAULT_C};

   test_action_action<N(testapi), WASM_TEST_ACTION("test_action", "read_action_normal")> test_action;
   copy_data((char*)&payload, sizeof(dummy_action), test_action.data); 
  
   auto trx = transaction();
   trx.actions.emplace_back(vector<permission_level>{{N(testapi), N(active)}}, test_action);
   trx.send(0);
}

void test_transaction::send_action_sender() {
   account_name cur_send;
   read_action_data( &cur_send, sizeof(account_name) );
   test_action_action<N(testapi), WASM_TEST_ACTION("test_action", "test_current_sender")> test_action;
   copy_data((char*)&cur_send, sizeof(account_name), test_action.data);

   auto trx = transaction();
   trx.actions.emplace_back(vector<permission_level>{{N(testapi), N(active)}}, test_action);
   trx.send(0);
}

void test_transaction::send_transaction_empty() {
   auto trx = transaction();
   trx.send(0);

   eosio_assert(false, "send_transaction_empty() should've thrown an error");
}

/**
 * cause failure due to a large transaction size
 */
void test_transaction::send_transaction_large() {
   auto trx = transaction();
   for (int i = 0; i < 32; i ++) {
      char large_message[1024];
      test_action_action<N(testapi), WASM_TEST_ACTION("test_action", "read_action_normal")> test_action;
      copy_data(large_message, 1024, test_action.data); 
      trx.actions.emplace_back(vector<permission_level>{{N(testapi), N(active)}}, test_action);
   }

   trx.send(0);

   eosio_assert(false, "send_transaction_large() should've thrown an error");
}
