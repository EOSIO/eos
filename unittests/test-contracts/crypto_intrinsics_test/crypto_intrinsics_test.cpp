#include <eosio/eosio.hpp>
#include <eosio/crypto.hpp>
#include <array>

extern "C"
{
	struct checksum {
		uint8_t hash[32];
	};

   struct ec_point {
      uint16_t curve;
      uint8_t  point[64];
   };

	__attribute__((eosio_wasm_import))
	void keccak(const char*, uint32_t, checksum*);
	__attribute__((eosio_wasm_import))
	void sha3(const char*, uint32_t, checksum*);
	__attribute__((eosio_wasm_import))
	void assert_keccak(const char*, uint32_t, const checksum*);
	__attribute__((eosio_wasm_import))
	void assert_sha3(const char*, uint32_t, const checksum*);
	__attribute__((eosio_wasm_import))
	int64_t ec_add(const ec_point*, const ec_point*, ec_point*);
}

static constexpr uint16_t r1_curve = 415;
static constexpr uint16_t k1_curve = 714;

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

	[[eosio::action]] void ecaddr1(std::vector<char> p, std::vector<char> q, std::vector<char> e) {
      ec_point a, b, r;
      memcpy(&a, p.data(), p.size());
      memcpy(&b, q.data(), q.size());

		int64_t res = ec_add(&a, &b, &r);
		eosio::check(res != -1, "ec_add failure");
		eosio::check(!memcmp(&r, e.data(), e.size()), "expected failure");
	}
};
