#include "legacydb.hpp"

using namespace std;
using namespace eosio;
using namespace eosio::internal_use_do_not_use;
using namespace legacydb;

const std::vector<char> data_empty{};
const std::vector<char> data_1{ 1, 2, 3, 4, 0, 4, 3, 2, 1 };
const std::vector<char> data_2{ 3, 4, 3, 4 };
const std::vector<char> data_3{ 3, 3, 3, 3, 3, 3, 3 };
const std::vector<char> data_4{ 4, 5, 6, 7 };

void store_i64(name scope, name table, name payer, uint64_t id, const std::vector<char>& data) {
   db_store_i64(scope.value, table.value, payer.value, id, data.data(), data.size());
}

int lowerbound_i64(name code, name scope, name table, uint64_t id) {
   return db_lowerbound_i64(code.value, scope.value, table.value, id);
}

std::vector<char> get_i64(int itr) {
   std::vector<char> result(db_get_i64(itr, nullptr, 0));
   db_get_i64(itr, result.data(), result.size());
   return result;
}

void check_lowerbound_i64(name code, name scope, name table, uint64_t id, int expected_itr,
                          const std::vector<char>& expected_data) {
   eosio::check(lowerbound_i64(code, scope, table, id) == expected_itr, "check_lowerbound_i64 failure: wrong iterator");
   eosio::check(get_i64(expected_itr) == expected_data, "check_lowerbound_i64 failure: wrong data");
}

void check_next_i64(int itr, int expected_itr, uint64_t expected_id) {
   uint64_t id       = 0;
   auto     next_itr = db_next_i64(itr, &id);
   eosio::check(next_itr == expected_itr, "check_next_i64 failure: wrong iterator. itr: " + to_string(itr) + " next: " +
                                                to_string(next_itr) + " expected: " + to_string(expected_itr) + "\n");
   if (expected_itr >= 0)
      eosio::check(id == expected_id, "check_next_i64 failure: wrong id. itr: " + to_string(itr) +
                                            " next: " + to_string(next_itr) + " id: " + to_string(id) +
                                            " expected: " + to_string(expected_id));
}

void legacydb_contract::write() {
   print("write\n");
   store_i64("scope1"_n, "table1"_n, get_self(), 20, data_empty);
   store_i64("scope1"_n, "table1"_n, get_self(), 21, data_1);
   store_i64("scope1"_n, "table1"_n, get_self(), 22, data_2);
   store_i64("scope1"_n, "table1"_n, get_self(), 23, data_3);
   store_i64("scope1"_n, "table1"_n, get_self(), 24, data_4);

   store_i64("scope1"_n, "table2"_n, get_self(), 30, data_4);
   store_i64("scope1"_n, "table2"_n, get_self(), 31, data_empty);
   store_i64("scope1"_n, "table2"_n, get_self(), 32, data_2);
   store_i64("scope1"_n, "table2"_n, get_self(), 33, data_1);
   store_i64("scope1"_n, "table2"_n, get_self(), 34, data_3);
}

void legacydb_contract::read() {
   print("read\n");

   // in order
   check_lowerbound_i64(get_self(), "scope1"_n, "table1"_n, 20, 0, data_empty);
   check_lowerbound_i64(get_self(), "scope1"_n, "table1"_n, 21, 1, data_1);
   check_lowerbound_i64(get_self(), "scope1"_n, "table1"_n, 22, 2, data_2);
   check_lowerbound_i64(get_self(), "scope1"_n, "table1"_n, 23, 3, data_3);
   check_lowerbound_i64(get_self(), "scope1"_n, "table1"_n, 24, 4, data_4);

   check_next_i64(0, 1, 21);
   check_next_i64(1, 2, 22);
   check_next_i64(2, 3, 23);
   check_next_i64(3, 4, 24);
   check_next_i64(4, -2, 0);
   check_next_i64(-2, -1, 0);
}

EOSIO_ACTION_DISPATCHER(legacydb::actions)
