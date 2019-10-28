#include <eosio/eosio.hpp>

extern "C"
{
	struct checksum {
		uint8_t hash[32];
	};
	__attribute__((eosio_wasm_import))
	void keccak(const char*, uint32_t, checksum*);
	__attribute__((eosio_wasm_import))
	void sha3(const char*, uint32_t, checksum*);
	__attribute__((eosio_wasm_import))
	void assert_keccak(const char*, uint32_t, const checksum*);
	__attribute__((eosio_wasm_import))
	void assert_sha3(const char*, uint32_t, const checksum*);
}

class[[eosio::contract]] crypto_intrinsics_test : public eosio::contract
{
public:
	using eosio::contract::contract;
    uint8_t from_hex( char c ) {
      if( c >= '0' && c <= '9' )
        return c - '0';
      if( c >= 'a' && c <= 'f' )
          return c - 'a' + 10;
      if( c >= 'A' && c <= 'F' )
          return c - 'A' + 10;
		eosio::check(false, "fail");
		return -1;
   }

	void from_hex( const std::string& hex_str, char* out_data, size_t out_data_len ) {
		for (int i=0, j=0; i < hex_str.size(); i += 2, j++)
			out_data[j] = (from_hex(hex_str[i]) << 4) | from_hex(hex_str[i+1]);
	}

	[[eosio::action]] void sha3(std::string s, std::string expected) {
		checksum test;
		::sha3(s.c_str(), s.size(), &test);
		::assert_sha3(s.c_str(), s.size(), &test);
		memset((char*)test.hash, 0, 32);
		from_hex(expected, (char*)test.hash, 32);
		::assert_sha3(s.c_str(), s.size(), &test);
	}
	[[eosio::action]] void keccak(std::string s, std::string expected) {
		checksum test;
		::keccak(s.c_str(), s.size(), &test);
		::assert_keccak(s.c_str(), s.size(), &test);
		memset((char*)test.hash, 0, 32);
		from_hex(expected, (char*)test.hash, 32);
		::assert_keccak(s.c_str(), s.size(), &test);
	}
};
