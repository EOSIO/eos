#pragma once

struct test_types {
  static const int test_id     = 1;
  static const int total_tests() { return 4; }

  static unsigned int test1();
  static unsigned int test2();
  static unsigned int test3();
  static unsigned int test4();
};
