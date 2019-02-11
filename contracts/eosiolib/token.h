#ifndef EOSIOLIB_TOKEN_H_
#define EOSIOLIB_TOKEN_H_

#include <eosiolib/types.h>
#ifdef __cplusplus
extern "C" {
#endif

void token_create( uint64_t issuer, void* maximum_supply, size_t size);
void token_issue( uint64_t to, void* quantity, size_t size1, const char* memo, size_t size2 );
void token_transfer( uint64_t from, uint64_t to, void* quantity, size_t size1, const char* memo, size_t size2);

#ifdef __cplusplus
}
#endif

#endif
