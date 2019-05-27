#include <array>
#include <vector>
#include <fc/crypto/public_key.hpp>

namespace eosio {

struct fido_device_impl;

struct fido_registration_result {
   fc::crypto::public_key pub_key;
   std::vector<uint8_t> credential_id;
};

struct fido_assertion_result {
   std::vector<uint8_t> auth_data;
   std::vector<uint8_t> der_signature;
};

class fido_device {
   public:
      //conenct to a connected FIDO2 device; throws on no device or on other error
      fido_device();
      ~fido_device();

      fido_registration_result do_registration(std::array<uint8_t, 32> client_data_hash);

      fido_assertion_result do_assertion(const fc::sha256& client_data_hash, const std::vector<uint8_t>& credential_id);

   private:
      std::unique_ptr<fido_device_impl> my;
};

}