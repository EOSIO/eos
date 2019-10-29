
#include <eosio/eosio.hpp>

#include "test_api.hpp"

void test_print::test_prints_l() {
  char ab[] = { 'a', 'b' };
  const char test[] = "test";
  printl(ab, 2);
  printl(ab, 1);
  printl(ab, 0);
  printl(test, sizeof(test)-1);
}

void test_print::test_prints() {
   eosio::print("ab");
   eosio::print("c\0test_prints");
   eosio::print("efg");
}

void test_print::test_printi() {
   eosio::print(0);
   eosio::print(556644);
   eosio::print(-1);
}

void test_print::test_printui() {
   eosio::print(0);
   eosio::print(556644);
   eosio::print((uint64_t)-1);
}

//void test_print::test_printi128() {
//  int128_t a(1);
//  int128_t b(0);
//  int128_t c(std::numeric_limits<int128_t>::lowest());
//  int128_t d(-87654323456);
//  printi128(&a);
//  prints("\n");
//  printi128(&b);
//  prints("\n");
//  printi128(&c);
//  prints("\n");
//  printi128(&d);
//  prints("\n");
//}

//void test_print::test_printui128() {
//  uint128_t a((uint128_t)-1);
//  uint128_t b(0);
//  uint128_t c(87654323456);
//  print(&a);
//  print("\n");
//  printui128(&b);
//  print("\n");
//  printui128(&c);
//  print("\n");
//}

void test_print::test_printn() {
   eosio::name{"1"}.print();
   eosio::name{"5"}.print();
   eosio::name{"a"}.print();
   eosio::name{"z"}.print();

   eosio::name{"abc"}.print();
   eosio::name{"123"}.print();

   eosio::name{"abc.123"}.print();
   eosio::name{"123.abc"}.print();

   eosio::name{"12345abcdefgj"}.print();
   eosio::name{"ijklmnopqrstj"}.print();
   eosio::name{"vwxyz.12345aj"}.print();

   eosio::name{"111111111111j"}.print();
   eosio::name{"555555555555j"}.print();
   eosio::name{"aaaaaaaaaaaaj"}.print();
   eosio::name{"zzzzzzzzzzzzj"}.print();
}


void test_print::test_printsf() {
   float x = 1.0f / 2.0f;
   float y = 5.0f * -0.75f;
   float z = 2e-6f / 3.0f;
   eosio::print(x);
   eosio::print("\n");
   eosio::print(y);
   eosio::print("\n");
   eosio::print(z);
   eosio::print("\n");
}

void test_print::test_printdf() {
   double x = 1.0 / 2.0;
   double y = 5.0 * -0.75;
   double z = 2e-6 / 3.0;
   eosio::print(x);
   eosio::print("\n");
   eosio::print(y);
   eosio::print("\n");
   eosio::print(z);
   eosio::print("\n");
}

//void test_print::test_printqf() {
//   long double x = 1.0l / 2.0l;
//   long double y = 5.0l * -0.75l;
//   long double z = 2e-6l / 3.0l;
//   printqf(&x);
//   prints("\n");
//   printqf(&y);
//   prints("\n");
//   printqf(&z);
//   prints("\n");
//}

void test_print::test_print_simple() {
    const std::string cvalue = "cvalue";
    eosio::print(cvalue);
    std::string value = "value";
    eosio::print(std::move(value));
}
