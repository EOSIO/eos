#include <eosio/eosio.hpp>

extern "C"
{
	__attribute__((eosio_wasm_import))
	int64_t ec_add(uint64_t, const char*, uint32_t, const char*, uint32_t);
}

class[[eosio::contract]] crypto_ec_add_test : public eosio::contract
{
public:
	using eosio::contract::contract;

   // testing protocol feature linkage
	[[eosio::action]] void ecadd() {
		::ec_add(0, nullptr, 0, nullptr, 0);
	}
};
