#ifndef __HELLO_H
#define __HELLO_H
	int get_info_(char *result,int length);
	int get_block_(int id,char *result,int length);
    int get_account_(char* name,char *result,int length);
    void create_account_( char* creator_,char* newaccount_,char* owner_key_,char* active_key_ );
	int create_key_(char *pub_,int pub_length,char *priv_,int priv_length);
    int get_transaction_(char *id,char* result,int length);
#endif


