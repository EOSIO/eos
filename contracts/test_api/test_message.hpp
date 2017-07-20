#pragma once

#define DUMMY_MESSAGE_DEFAULT_A 0x45
#define DUMMY_MESSAGE_DEFAULT_B 0xab11cd1244556677
#define DUMMY_MESSAGE_DEFAULT_C 0x7451ae12

#pragma pack(push, 1)
struct dummy_message {
  char a; //1
  unsigned long long b; //8
  int  c; //4
};
#pragma pack(pop)

static_assert( sizeof(dummy_message) == 13 , "unexpected packing" );

struct test_message {
  static const int test_id = 2;
  static const int total_tests() { return 9; }

  static unsigned int test1();
  static unsigned int test2();
  static unsigned int test3();
  static unsigned int test4();
  static unsigned int test5();
  static unsigned int test6();
  static unsigned int test7();
  static unsigned int test8();
  static unsigned int test9();
};

