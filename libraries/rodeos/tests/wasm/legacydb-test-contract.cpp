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

#define WRAP_SECONDARY(NAME, TYPE)                                                                                     \
   struct NAME {                                                                                                       \
      static int32_t store(name scope, name table, name payer, uint64_t id, const TYPE& secondary) {                   \
         return db_##NAME##_store(scope.value, table.value, payer.value, id, &secondary);                              \
      }                                                                                                                \
      static void update(int32_t iterator, name payer, const TYPE& secondary) {                                        \
         return db_##NAME##_update(iterator, payer.value, &secondary);                                                 \
      }                                                                                                                \
      static void    remove(int32_t iterator) { return db_##NAME##_remove(iterator); }                                 \
      static int32_t find_secondary(name code, name scope, name table, const TYPE& secondary, uint64_t& primary) {     \
         return db_##NAME##_find_secondary(code.value, scope.value, table.value, &secondary, &primary);                \
      }                                                                                                                \
      static int32_t find_primary(name code, name scope, name table, TYPE& secondary, uint64_t primary) {              \
         return db_##NAME##_find_primary(code.value, scope.value, table.value, &secondary, primary);                   \
      }                                                                                                                \
      static int32_t lowerbound(name code, name scope, name table, TYPE& secondary, uint64_t& primary) {               \
         return db_##NAME##_lowerbound(code.value, scope.value, table.value, &secondary, &primary);                    \
      }                                                                                                                \
      static int32_t upperbound(name code, name scope, name table, TYPE& secondary, uint64_t& primary) {               \
         return db_##NAME##_upperbound(code.value, scope.value, table.value, &secondary, &primary);                    \
      }                                                                                                                \
      static int32_t end(name code, name scope, name table) {                                                          \
         return db_##NAME##_end(code.value, scope.value, table.value);                                                 \
      }                                                                                                                \
      static int32_t next(int32_t iterator, uint64_t& primary) { return db_##NAME##_next(iterator, &primary); }        \
      static int32_t previous(int32_t iterator, uint64_t& primary) {                                                   \
         return db_##NAME##_previous(iterator, &primary);                                                              \
      }                                                                                                                \
      static void check_lowerbound(name code, name scope, name table, int expected_itr, uint64_t expected_primary,     \
                                   const TYPE& secondary_search, const TYPE& expected_secondary) {                     \
         uint64_t primary   = 0xfeedbeef;                                                                              \
         auto     secondary = secondary_search;                                                                        \
         auto     itr       = lowerbound(code, scope, table, secondary, primary);                                      \
         eosio::check(itr == expected_itr && primary == expected_primary && secondary == expected_secondary,           \
                      "check_lowerbound failure: code: " + code.to_string() + " scope: " + scope.to_string() +         \
                            " table: " + table.to_string() +                                                           \
                            " secondary_search: " + convert_to_json(secondary_search) +                                \
                            " itr: " + convert_to_json(itr) + " expected_itr: " + convert_to_json(expected_itr) +      \
                            " secondary: " + convert_to_json(secondary) + " expected_secondary: " +                    \
                            convert_to_json(expected_secondary) + " primary: " + convert_to_json(primary) +            \
                            " expected_primary: " + convert_to_json(expected_primary) + "\n");                         \
      }                                                                                                                \
   };

WRAP_SECONDARY(idx64, uint64_t);
WRAP_SECONDARY(idx128, uint128_t);
WRAP_SECONDARY(idx_double, double);

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

name p(name prefix, name suffix) { return name{ prefix.value | (suffix.value >> 10) }; }

void store_multiple(name scope, name table, name payer, uint64_t id, const std::vector<char>& data, uint64_t i64,
                    uint128_t i128) {
   store_i64(scope, table, payer, id, data);
   idx64::store(p("a"_n, scope), p("a"_n, table), payer, id, i64);
   idx128::store(p("b"_n, scope), p("b"_n, table), payer, id, i128);
}

void check_lowerbound_multiple(name code, name scope, name table, int expected_itr, uint64_t expected_primary,
                               uint64_t idx64_search, uint64_t idx64_expected, uint128_t idx128_search,
                               uint128_t idx128_expected) {
   idx64::check_lowerbound(code, p("a"_n, scope), p("a"_n, table), expected_itr, expected_primary, idx64_search,
                           idx64_expected);
   idx128::check_lowerbound(code, p("b"_n, scope), p("b"_n, table), expected_itr, expected_primary, idx128_search,
                            idx128_expected);
}

auto big(uint64_t msb, uint64_t lsb) { return (uint128_t(msb) << 64) | lsb; }

auto big(uint64_t lsb) { return big(0xffff'ffff'ffff'ffff, lsb); }

void legacydb_contract::write() {
   print("write\n");
   store_multiple("scope1"_n, "table1"_n, get_self(), 20, data_0, 0x2020, 0x2020'2020);
   store_multiple("scope1"_n, "table1"_n, get_self(), 21, data_1, 0x2121, 0x2121'2121);
   store_multiple("scope1"_n, "table1"_n, get_self(), 22, data_2, 0x2222, 0x2222'2222);
   store_multiple("scope1"_n, "table1"_n, get_self(), 23, data_3, 0x2323, 0x2323'2323);
   store_multiple("scope1"_n, "table1"_n, get_self(), 24, data_4, 0x2424, 0x2424'2424);

   store_multiple("scope1"_n, "table2"_n, get_self(), 30, data_4, 0x3030, 0x3030'3030);
   store_multiple("scope1"_n, "table2"_n, get_self(), 31, data_0, 0x3131, 0x3131'3131);
   store_multiple("scope1"_n, "table2"_n, get_self(), 32, data_2, 0x3232, 0x3232'3232);
   store_multiple("scope1"_n, "table2"_n, get_self(), 33, data_1, 0x3333, 0x3333'3333);
   store_multiple("scope1"_n, "table2"_n, get_self(), 34, data_3, 0x3434, 0x3434'3434);

   store_multiple("scope1"_n, "table3"_n, get_self(), 40, data_0, 0x4040, 0x4040'4040);
   store_multiple("scope1"_n, "table3"_n, get_self(), 41, data_1, 0x4141, 0x4141'4141);
   store_multiple("scope1"_n, "table3"_n, get_self(), 42, data_2, 0x4242, 0x4242'4242);
   store_multiple("scope1"_n, "table3"_n, get_self(), 43, data_3, 0x4343, 0x4343'4343);
   store_multiple("scope1"_n, "table3"_n, get_self(), 44, data_4, 0x4444, 0x4444'4444);

   store_multiple("scope2"_n, "table1"_n, get_self(), 50, data_0, 0x5050, 0x5050'5050);
   store_multiple("scope2"_n, "table1"_n, get_self(), 51, data_1, 0x5151, 0x5151'5151);
   store_multiple("scope2"_n, "table1"_n, get_self(), 52, data_2, 0x5252, 0x5252'5252);
   store_multiple("scope2"_n, "table1"_n, get_self(), 53, data_3, 0x5353, 0x5353'5353);
   store_multiple("scope2"_n, "table1"_n, get_self(), 54, data_4, 0x5454, 0x5454'5454);

   store_multiple("scope2"_n, "atable"_n, get_self(), 60, data_0, 0x6060, 0x6060'6060);
   store_multiple("scope2"_n, "atable"_n, get_self(), 61, data_1, 0x6161, 0x6161'6161);
   store_multiple("scope2"_n, "atable"_n, get_self(), 62, data_2, 0x6262, 0x6262'6262);
   store_multiple("scope2"_n, "atable"_n, get_self(), 63, data_3, 0x6363, 0x6363'6363);
   store_multiple("scope2"_n, "atable"_n, get_self(), 64, data_4, 0x6464, 0x6464'6464);
   store_multiple("scope2"_n, "atable"_n, get_self(), 0xffff'ffff'ffff'fffe, data_2, 0xffff'ffff'ffff'fffe,
                  big(0xffff'ffff'ffff'fffe));
   store_multiple("scope2"_n, "atable"_n, get_self(), 0xffff'ffff'ffff'ffff, data_3, 0xffff'ffff'ffff'ffff,
                  big(0xffff'ffff'ffff'ffff));

   store_multiple("scope.x"_n, "table1"_n, get_self(), 54, data_0, 0x5454, 0x5454'5454);
   store_multiple("scope.x"_n, "table2"_n, get_self(), 54, data_1, 0x5454, 0x5454'5454);

   idx64::store("nope"_n, "just.2nd"_n, get_self(), 42, 42);
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

   // clang-format off
   check_lowerbound_multiple(get_self(), "scope1"_n, "table1"_n, 0,  20,           0x0,    0x2020,  0x0,         0x2020'2020);
   check_lowerbound_multiple(get_self(), "scope1"_n, "table1"_n, 0,  20,           0x2020, 0x2020,  0x2020'2020, 0x2020'2020); // reuse existing itr
   check_lowerbound_multiple(get_self(), "scope1"_n, "table1"_n, 1,  21,           0x2121, 0x2121,  0x2121'2121, 0x2121'2121);
   check_lowerbound_multiple(get_self(), "scope1"_n, "table1"_n, 1,  21,           0x2121, 0x2121,  0x2121'2121, 0x2121'2121); // reuse existing itr
   check_lowerbound_multiple(get_self(), "scope1"_n, "table1"_n, 2,  22,           0x2222, 0x2222,  0x2222'2222, 0x2222'2222);
   check_lowerbound_multiple(get_self(), "scope1"_n, "table1"_n, 3,  23,           0x2323, 0x2323,  0x2323'2323, 0x2323'2323);
   check_lowerbound_multiple(get_self(), "scope1"_n, "table1"_n, 4,  24,           0x2424, 0x2424,  0x2424'2424, 0x2424'2424);
   check_lowerbound_multiple(get_self(), "scope1"_n, "table1"_n, -2, 0xfeedbeef,   0x2525, 0x2525,  0x2525'2525, 0x2525'2525);
   check_lowerbound_multiple(get_self(), "nope"_n,   "nada"_n,   -1, 0xfeedbeef,   0x2525, 0x2525,  0x2525'2525, 0x2525'2525);
   // clang-format on

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

   // clang-format off
   check_lowerbound_multiple(get_self(), "scope1"_n, "table2"_n,  5, 31,         0x3131, 0x3131,   0x3131'3131, 0x3131'3131);
   check_lowerbound_multiple(get_self(), "scope1"_n, "table2"_n,  6, 33,         0x3333, 0x3333,   0x3333'3333, 0x3333'3333);
   check_lowerbound_multiple(get_self(), "scope1"_n, "table2"_n,  7, 32,         0x3232, 0x3232,   0x3232'3232, 0x3232'3232);
   check_lowerbound_multiple(get_self(), "scope1"_n, "table2"_n,  8, 34,         0x3434, 0x3434,   0x3434'3434, 0x3434'3434);
   check_lowerbound_multiple(get_self(), "scope1"_n, "table2"_n,  9, 30,         0x3030, 0x3030,   0x3030'3030, 0x3030'3030);
   check_lowerbound_multiple(get_self(), "scope1"_n, "table2"_n,  9, 30,         0x0000, 0x3030,   0x0000'0000, 0x3030'3030);
   check_lowerbound_multiple(get_self(), "scope1"_n, "table2"_n, -3, 0xfeedbeef, 0x3535, 0x3535,   0x3535'3535, 0x3535'3535);
   // clang-format on

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
   // creates iterators -7, -8
   check_end_i64(get_self(), "scope1"_n, "table1"_n, -2);
   check_end_i64(get_self(), "scope1"_n, "table2"_n, -3);
   check_end_i64(get_self(), "scope1"_n, "table3"_n, -4);
   check_end_i64(get_self(), "scope2"_n, "table1"_n, -5);
   check_end_i64(get_self(), "scope2"_n, "atable"_n, -6);
   check_end_i64(get_self(), "scope.x"_n, "table2"_n, -7); // not searched for yet
   check_end_i64(get_self(), "scope.x"_n, "table1"_n, -8); // not searched for yet
   check_end_i64(get_self(), "nope"_n, "nada"_n, -1);

   // non-existing table
   check_lowerbound_i64(get_self(), "nope"_n, "nada"_n, 25, -1, {});
   check_find_i64(get_self(), "nope"_n, "nada"_n, 25, -1, {});
   check_upperbound_i64(get_self(), "nope"_n, "nada"_n, 25, -1, {});
   check_end_i64(get_self(), "nope"_n, "nada"_n, -1);

   // empty table
   // creates iterators -9
   check_lowerbound_i64(get_self(), "nope"_n, "just.2nd"_n, 42, -9, {});
   check_find_i64(get_self(), "nope"_n, "just.2nd"_n, 42, -9, {});
   check_upperbound_i64(get_self(), "nope"_n, "just.2nd"_n, 42, -9, {});
   check_end_i64(get_self(), "nope"_n, "just.2nd"_n, -9);
} // legacydb_contract::read()

EOSIO_ACTION_DISPATCHER(legacydb::actions)
