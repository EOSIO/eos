#include <microfido/microfido.hpp>

using namespace eosio;

int main() {

   fido_registration_result reg;

   try {
      fido_device fido;
      std::array<uint8_t, 32> cdh; //Contents don't really matter for purposes of this
      std::cout << "Touch FIDO2 device to complete key creation..." << std::endl;
      reg = fido.do_registration(cdh);
   }
   catch(...) {
      //XXX print error
      std::cerr << "Unable to generate key on FIDO2 device" << std::endl;
      return 1;
   }

   std::cout << "Key created. Add the following line to your keosd configuration to use:" << std::endl;
   std::cout << "webauthn-key=" << std::string(reg.pub_key) << "=" << fc::to_hex((char*)reg.credential_id.data(), reg.credential_id.size()) << std::endl;

   return 0;
}