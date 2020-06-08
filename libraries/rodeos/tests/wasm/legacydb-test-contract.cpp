#include "legacydb-test-contract.hpp"

using namespace std;
using namespace eosio;
using namespace eosio::internal_use_do_not_use;
using namespace legacydb;

const std::vector<char> data_0{ 9, 7, 6 };
const std::vector<char> data_1{ 1, 2, 3, 4, 0, 4, 3, 2, 1 };
const std::vector<char> data_2{ 3, 4, 3, 4 };
const std::vector<char> data_3{};
const std::vector<char> data_4{ 4, 5, 6, 7 };

void store_i64(name scope, name table, name payer, uint64_t id, const std::vector<char>& data) {
   db_store_i64(scope.value, table.value, payer.value, id, data.data(), data.size());
}

int find_i64(name code, name scope, name table, uint64_t id) {
   return db_find_i64(code.value, scope.value, table.value, id);
}

int lowerbound_i64(name code, name scope, name table, uint64_t id) {
   return db_lowerbound_i64(code.value, scope.value, table.value, id);
}

int upperbound_i64(name code, name scope, name table, uint64_t id) {
   return db_upperbound_i64(code.value, scope.value, table.value, id);
}

int end_i64(name code, name scope, name table) { return db_end_i64(code.value, scope.value, table.value); }

std::vector<char> get_i64(int itr) {
   std::vector<char> result(db_get_i64(itr, nullptr, 0));
   db_get_i64(itr, result.data(), result.size());
   return result;
}

void check_lowerbound_i64(name code, name scope, name table, uint64_t id, int expected_itr,
                          const std::vector<char>& expected_data) {
   auto itr = lowerbound_i64(code, scope, table, id);
   eosio::check(itr == expected_itr, "check_lowerbound_i64 failure: wrong iterator. code: " + code.to_string() +
                                           " scope: " + scope.to_string() + " table: " + table.to_string() +
                                           " id: " + to_string(id) + " itr: " + to_string(itr) +
                                           " expected_itr: " + to_string(expected_itr) + "\n");
   if (expected_itr >= 0)
      eosio::check(get_i64(expected_itr) == expected_data, "check_lowerbound_i64 failure: wrong data");
}

void check_upperbound_i64(name code, name scope, name table, uint64_t id, int expected_itr,
                          const std::vector<char>& expected_data) {
   auto itr = upperbound_i64(code, scope, table, id);
   eosio::check(itr == expected_itr, "check_upperbound_i64 failure: wrong iterator. code: " + code.to_string() +
                                           " scope: " + scope.to_string() + " table: " + table.to_string() +
                                           " id: " + to_string(id) + " itr: " + to_string(itr) +
                                           " expected_itr: " + to_string(expected_itr) + "\n");
   if (expected_itr >= 0)
      eosio::check(get_i64(expected_itr) == expected_data, "check_upperbound_i64 failure: wrong data");
}

void check_find_i64(name code, name scope, name table, uint64_t id, int expected_itr,
                    const std::vector<char>& expected_data) {
   auto itr = find_i64(code, scope, table, id);
   eosio::check(itr == expected_itr, "check_find_i64 failure: wrong iterator. code: " + code.to_string() +
                                           " scope: " + scope.to_string() + " table: " + table.to_string() +
                                           " id: " + to_string(id) + " itr: " + to_string(itr) +
                                           " expected_itr: " + to_string(expected_itr) + "\n");
   if (expected_itr >= 0)
      eosio::check(get_i64(expected_itr) == expected_data, "check_find_i64 failure: wrong data");
}

void check_end_i64(name code, name scope, name table, int expected_itr) {
   auto itr = end_i64(code, scope, table);
   eosio::check(itr == expected_itr, "check_end_i64 failure: wrong iterator. code: " + code.to_string() +
                                           " scope: " + scope.to_string() + " table: " + table.to_string() + " itr: " +
                                           to_string(itr) + " expected_itr: " + to_string(expected_itr) + "\n");
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

void check_prev_i64(int itr, int expected_itr, uint64_t expected_id) {
   uint64_t id       = 0;
   auto     next_itr = db_previous_i64(itr, &id);
   eosio::check(next_itr == expected_itr, "check_prev_i64 failure: wrong iterator. itr: " + to_string(itr) + " prev: " +
                                                to_string(next_itr) + " expected: " + to_string(expected_itr) + "\n");
   if (expected_itr >= 0)
      eosio::check(id == expected_id, "check_prev_i64 failure: wrong id. itr: " + to_string(itr) +
                                            " prev: " + to_string(next_itr) + " id: " + to_string(id) +
                                            " expected: " + to_string(expected_id));
}

void legacydb_contract::write() {
   print("write\n");
   store_i64("scope1"_n, "table1"_n, get_self(), 20, data_0);
   store_i64("scope1"_n, "table1"_n, get_self(), 21, data_1);
   store_i64("scope1"_n, "table1"_n, get_self(), 22, data_2);
   store_i64("scope1"_n, "table1"_n, get_self(), 23, data_3);
   store_i64("scope1"_n, "table1"_n, get_self(), 24, data_4);

   store_i64("scope1"_n, "table2"_n, get_self(), 30, data_4);
   store_i64("scope1"_n, "table2"_n, get_self(), 31, data_0);
   store_i64("scope1"_n, "table2"_n, get_self(), 32, data_2);
   store_i64("scope1"_n, "table2"_n, get_self(), 33, data_1);
   store_i64("scope1"_n, "table2"_n, get_self(), 34, data_3);

   store_i64("scope1"_n, "table3"_n, get_self(), 40, data_0);
   store_i64("scope1"_n, "table3"_n, get_self(), 41, data_1);
   store_i64("scope1"_n, "table3"_n, get_self(), 42, data_2);
   store_i64("scope1"_n, "table3"_n, get_self(), 43, data_3);
   store_i64("scope1"_n, "table3"_n, get_self(), 44, data_4);

   store_i64("scope2"_n, "table1"_n, get_self(), 50, data_0);
   store_i64("scope2"_n, "table1"_n, get_self(), 51, data_1);
   store_i64("scope2"_n, "table1"_n, get_self(), 52, data_2);
   store_i64("scope2"_n, "table1"_n, get_self(), 53, data_3);
   store_i64("scope2"_n, "table1"_n, get_self(), 54, data_4);

   store_i64("scope2"_n, "atable"_n, get_self(), 60, data_0);
   store_i64("scope2"_n, "atable"_n, get_self(), 61, data_1);
   store_i64("scope2"_n, "atable"_n, get_self(), 62, data_2);
   store_i64("scope2"_n, "atable"_n, get_self(), 63, data_3);
   store_i64("scope2"_n, "atable"_n, get_self(), 64, data_4);
   store_i64("scope2"_n, "atable"_n, get_self(), 0xffff'ffff'ffff'fffe, data_2);
   store_i64("scope2"_n, "atable"_n, get_self(), 0xffff'ffff'ffff'ffff, data_3);

   store_i64("scope.x"_n, "table1"_n, get_self(), 54, data_0);
   store_i64("scope.x"_n, "table2"_n, get_self(), 54, data_1);
} // legacydb_contract::write()

void legacydb_contract::read() {
   print("read\n");

   // find items in order -> produces iterator indexes in order
   // creates iterators -2, 0 - 4
   check_lowerbound_i64(get_self(), "scope1"_n, "table1"_n, 0, 0, data_0);
   check_lowerbound_i64(get_self(), "scope1"_n, "table1"_n, 20, 0, data_0); // reuse existing itr
   check_lowerbound_i64(get_self(), "scope1"_n, "table1"_n, 21, 1, data_1);
   check_lowerbound_i64(get_self(), "scope1"_n, "table1"_n, 21, 1, data_1); // reuse existing itr
   check_lowerbound_i64(get_self(), "scope1"_n, "table1"_n, 22, 2, data_2);
   check_lowerbound_i64(get_self(), "scope1"_n, "table1"_n, 23, 3, data_3);
   check_lowerbound_i64(get_self(), "scope1"_n, "table1"_n, 24, 4, data_4);
   check_lowerbound_i64(get_self(), "scope1"_n, "table1"_n, 25, -2, {});
   check_lowerbound_i64(get_self(), "nope"_n, "nada"_n, 25, -1, {});

   check_find_i64(get_self(), "scope1"_n, "table1"_n, 0, -2, {});
   check_find_i64(get_self(), "scope1"_n, "table1"_n, 20, 0, data_0);
   check_find_i64(get_self(), "scope1"_n, "table1"_n, 21, 1, data_1);
   check_find_i64(get_self(), "scope1"_n, "table1"_n, 22, 2, data_2);
   check_find_i64(get_self(), "scope1"_n, "table1"_n, 23, 3, data_3);
   check_find_i64(get_self(), "scope1"_n, "table1"_n, 24, 4, data_4);
   check_find_i64(get_self(), "scope1"_n, "table1"_n, 25, -2, {});
   check_find_i64(get_self(), "nope"_n, "nada"_n, 25, -1, {});

   check_upperbound_i64(get_self(), "scope1"_n, "table1"_n, 0, 0, data_0);
   check_upperbound_i64(get_self(), "scope1"_n, "table1"_n, 19, 0, data_0);
   check_upperbound_i64(get_self(), "scope1"_n, "table1"_n, 20, 1, data_1);
   check_upperbound_i64(get_self(), "scope1"_n, "table1"_n, 21, 2, data_2);
   check_upperbound_i64(get_self(), "scope1"_n, "table1"_n, 22, 3, data_3);
   check_upperbound_i64(get_self(), "scope1"_n, "table1"_n, 23, 4, data_4);
   check_upperbound_i64(get_self(), "scope1"_n, "table1"_n, 24, -2, {});
   check_upperbound_i64(get_self(), "scope1"_n, "table1"_n, 25, -2, {});
   check_upperbound_i64(get_self(), "scope1"_n, "table1"_n, 0xffff'ffff'ffff'fffe, -2, {});
   check_upperbound_i64(get_self(), "scope1"_n, "table1"_n, 0xffff'ffff'ffff'ffff, -2, {});
   check_upperbound_i64(get_self(), "nope"_n, "nada"_n, 25, -1, {});

   check_next_i64(0, 1, 21);
   check_next_i64(1, 2, 22);
   check_next_i64(2, 3, 23);
   check_next_i64(3, 4, 24);
   check_next_i64(4, -2, 0);
   check_next_i64(-2, -1, 0);

   check_prev_i64(-2, 4, 24);
   check_prev_i64(4, 3, 23);
   check_prev_i64(3, 2, 22);
   check_prev_i64(2, 1, 21);
   check_prev_i64(1, 0, 20);
   check_prev_i64(0, -1, 0);

   // find items out of order -> produces iterator indexes out of order
   // creates iterators -3, 5 - 9
   check_find_i64(get_self(), "scope1"_n, "table2"_n, 31, 5, data_0);
   check_find_i64(get_self(), "scope1"_n, "table2"_n, 33, 6, data_1);
   check_find_i64(get_self(), "scope1"_n, "table2"_n, 32, 7, data_2);
   check_find_i64(get_self(), "scope1"_n, "table2"_n, 34, 8, data_3);
   check_find_i64(get_self(), "scope1"_n, "table2"_n, 30, 9, data_4);
   check_find_i64(get_self(), "scope1"_n, "table2"_n, 0, -3, {});
   check_find_i64(get_self(), "scope1"_n, "table2"_n, 35, -3, {});

   check_lowerbound_i64(get_self(), "scope1"_n, "table2"_n, 31, 5, data_0);
   check_lowerbound_i64(get_self(), "scope1"_n, "table2"_n, 33, 6, data_1);
   check_lowerbound_i64(get_self(), "scope1"_n, "table2"_n, 32, 7, data_2);
   check_lowerbound_i64(get_self(), "scope1"_n, "table2"_n, 34, 8, data_3);
   check_lowerbound_i64(get_self(), "scope1"_n, "table2"_n, 30, 9, data_4);
   check_lowerbound_i64(get_self(), "scope1"_n, "table2"_n, 0, 9, data_4);
   check_lowerbound_i64(get_self(), "scope1"_n, "table2"_n, 35, -3, {});

   check_next_i64(9, 5, 31);
   check_next_i64(5, 7, 32);
   check_next_i64(7, 6, 33);
   check_next_i64(6, 8, 34);
   check_next_i64(8, -3, 0);
   check_next_i64(-3, -1, 0);

   check_prev_i64(-3, 8, 34);
   check_prev_i64(8, 6, 33);
   check_prev_i64(6, 7, 32);
   check_prev_i64(7, 5, 31);
   check_prev_i64(5, 9, 30);
   check_prev_i64(9, -1, 0);

   // iterate through table that has been only partially searched through
   // creates iterators -4, 10 - 14
   check_lowerbound_i64(get_self(), "scope1"_n, "table3"_n, 40, 10, data_0);
   check_lowerbound_i64(get_self(), "scope1"_n, "table3"_n, 43, 11, data_3);
   check_next_i64(10, 12, 41);
   check_next_i64(12, 13, 42);
   check_next_i64(13, 11, 43);
   check_next_i64(11, 14, 44);
   check_next_i64(14, -4, 0);
   check_next_i64(-4, -1, 0);

   check_prev_i64(-4, 14, 44);
   check_prev_i64(14, 11, 43);
   check_prev_i64(11, 13, 42);
   check_prev_i64(13, 12, 41);
   check_prev_i64(12, 10, 40);
   check_prev_i64(10, -1, 0);

   check_lowerbound_i64(get_self(), "scope1"_n, "table3"_n, 40, 10, data_0);
   check_lowerbound_i64(get_self(), "scope1"_n, "table3"_n, 0, 10, data_0);
   check_lowerbound_i64(get_self(), "scope1"_n, "table3"_n, 41, 12, data_1);
   check_lowerbound_i64(get_self(), "scope1"_n, "table3"_n, 42, 13, data_2);
   check_lowerbound_i64(get_self(), "scope1"_n, "table3"_n, 43, 11, data_3);
   check_lowerbound_i64(get_self(), "scope1"_n, "table3"_n, 44, 14, data_4);
   check_lowerbound_i64(get_self(), "scope1"_n, "table3"_n, 45, -4, {});

   check_find_i64(get_self(), "scope1"_n, "table3"_n, 40, 10, data_0);
   check_find_i64(get_self(), "scope1"_n, "table3"_n, 0, -4, {});
   check_find_i64(get_self(), "scope1"_n, "table3"_n, 41, 12, data_1);
   check_find_i64(get_self(), "scope1"_n, "table3"_n, 42, 13, data_2);
   check_find_i64(get_self(), "scope1"_n, "table3"_n, 43, 11, data_3);
   check_find_i64(get_self(), "scope1"_n, "table3"_n, 44, 14, data_4);
   check_find_i64(get_self(), "scope1"_n, "table3"_n, 45, -4, {});

   // iterate (reverse) through table that has been only partially searched through
   // creates iterators -5, 15 - 19
   check_find_i64(get_self(), "scope2"_n, "table1"_n, 50, 15, data_0);
   check_find_i64(get_self(), "scope2"_n, "table1"_n, 53, 16, data_3);
   check_prev_i64(-5, 17, 54);
   check_prev_i64(17, 16, 53);
   check_prev_i64(16, 18, 52);
   check_prev_i64(18, 19, 51);
   check_prev_i64(19, 15, 50);
   check_prev_i64(15, -1, 0);

   check_next_i64(15, 19, 51);
   check_next_i64(19, 18, 52);
   check_next_i64(18, 16, 53);
   check_next_i64(16, 17, 54);
   check_next_i64(17, -5, 0);
   check_next_i64(-5, -1, 0);

   check_find_i64(get_self(), "scope2"_n, "table1"_n, 0, -5, {});
   check_find_i64(get_self(), "scope2"_n, "table1"_n, 50, 15, data_0);
   check_find_i64(get_self(), "scope2"_n, "table1"_n, 51, 19, data_1);
   check_find_i64(get_self(), "scope2"_n, "table1"_n, 52, 18, data_2);
   check_find_i64(get_self(), "scope2"_n, "table1"_n, 53, 16, data_3);
   check_find_i64(get_self(), "scope2"_n, "table1"_n, 54, 17, data_4);
   check_find_i64(get_self(), "scope2"_n, "table1"_n, 55, -5, {});

   check_lowerbound_i64(get_self(), "scope2"_n, "table1"_n, 0, 15, data_0);
   check_lowerbound_i64(get_self(), "scope2"_n, "table1"_n, 50, 15, data_0);
   check_lowerbound_i64(get_self(), "scope2"_n, "table1"_n, 51, 19, data_1);
   check_lowerbound_i64(get_self(), "scope2"_n, "table1"_n, 52, 18, data_2);
   check_lowerbound_i64(get_self(), "scope2"_n, "table1"_n, 53, 16, data_3);
   check_lowerbound_i64(get_self(), "scope2"_n, "table1"_n, 54, 17, data_4);
   check_lowerbound_i64(get_self(), "scope2"_n, "table1"_n, 55, -5, {});

   // find items out of order using upperbound
   // creates iterators -6, 20 - 26
   check_upperbound_i64(get_self(), "scope2"_n, "atable"_n, 63, 20, data_4);
   check_upperbound_i64(get_self(), "scope2"_n, "atable"_n, 60, 21, data_1);
   check_upperbound_i64(get_self(), "scope2"_n, "atable"_n, 0xffff'ffff'ffff'fffe, 22, data_3);
   check_upperbound_i64(get_self(), "scope2"_n, "atable"_n, 60, 21, data_1); // reuse existing itr
   check_upperbound_i64(get_self(), "scope2"_n, "atable"_n, 61, 23, data_2);
   check_upperbound_i64(get_self(), "scope2"_n, "atable"_n, 0xffff'ffff'ffff'ffff, -6, {});
   check_upperbound_i64(get_self(), "scope2"_n, "atable"_n, 59, 24, data_0);
   check_upperbound_i64(get_self(), "scope2"_n, "atable"_n, 58, 24, data_0); // reuse existing itr
   check_upperbound_i64(get_self(), "scope2"_n, "atable"_n, 0xffff'ffff'ffff'fffd, 25, data_2);
   check_upperbound_i64(get_self(), "scope2"_n, "atable"_n, 62, 26, data_3);

   check_lowerbound_i64(get_self(), "scope2"_n, "atable"_n, 64, 20, data_4);
   check_lowerbound_i64(get_self(), "scope2"_n, "atable"_n, 65, 25, data_2);
   check_lowerbound_i64(get_self(), "scope2"_n, "atable"_n, 61, 21, data_1);
   check_lowerbound_i64(get_self(), "scope2"_n, "atable"_n, 0xffff'ffff'ffff'ffff, 22, data_3);
   check_lowerbound_i64(get_self(), "scope2"_n, "atable"_n, 62, 23, data_2);
   check_lowerbound_i64(get_self(), "scope2"_n, "atable"_n, 60, 24, data_0);
   check_lowerbound_i64(get_self(), "scope2"_n, "atable"_n, 59, 24, data_0);
   check_lowerbound_i64(get_self(), "scope2"_n, "atable"_n, 0xffff'ffff'ffff'fffe, 25, data_2);
   check_lowerbound_i64(get_self(), "scope2"_n, "atable"_n, 0xffff'ffff'ffff'fffd, 25, data_2);
   check_lowerbound_i64(get_self(), "scope2"_n, "atable"_n, 63, 26, data_3);

   // check end
   check_end_i64(get_self(), "scope1"_n, "table1"_n, -2);
   check_end_i64(get_self(), "scope1"_n, "table2"_n, -3);
   check_end_i64(get_self(), "scope1"_n, "table3"_n, -4);
   check_end_i64(get_self(), "scope2"_n, "table1"_n, -5);
   check_end_i64(get_self(), "scope2"_n, "atable"_n, -6);
   check_end_i64(get_self(), "scope.x"_n, "table2"_n, -7); // not searched for yet
   check_end_i64(get_self(), "scope.x"_n, "table1"_n, -8); // not searched for yet
   check_end_i64(get_self(), "nope"_n, "nada"_n, -1);
} // legacydb_contract::read()

EOSIO_ACTION_DISPATCHER(legacydb::actions)
