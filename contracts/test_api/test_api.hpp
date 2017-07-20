#pragma once

#define WASM_TEST_ERROR_CODE *((unsigned int *)((1<<16) - 2*sizeof(unsigned int)))
#define WASM_TEST_ERROR_MESSAGE *((unsigned int *)((1<<16) - 1*sizeof(unsigned int)))

#define WASM_TEST_FAIL 0xDEADBEEF
#define WASM_TEST_PASS 0xB0CABACA

#define WASM_ASSERT(m, message) if(!(m)) { WASM_TEST_ERROR_MESSAGE=(unsigned int)message; return WASM_TEST_FAIL; }

typedef unsigned long long u64;
#define WASM_TEST_HANDLER(C, N) if( C::test_id == (action >> 32) && N == (action & (u64(-1) >> 32)) ) { WASM_TEST_ERROR_CODE = C::test##N(); }