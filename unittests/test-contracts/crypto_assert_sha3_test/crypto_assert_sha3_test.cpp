#include <eosio/eosio.hpp>

extern "C"
{
	struct checksum {
		uint8_t hash[32];
	};
	__attribute__((eosio_wasm_import))
	void assert_sha3(const char*, uint32_t, const checksum*);
}

class[[eosio::contract]] crypto_assert_sha3_test : public eosio::contract
{
public:
	using eosio::contract::contract;

   // testing protocol feature linkage
	[[eosio::action]] void sha3() {
		checksum test;
		::assert_sha3("testing", 7, &test);
	}
};
