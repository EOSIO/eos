#ifdef NON_VALIDATING_TEST
#define TESTER tester
#define ROCKSDB_TESTER rocksdb_tester
#else
#define TESTER validating_tester
#define ROCKSDB_TESTER rocksdb_validating_tester
#endif
