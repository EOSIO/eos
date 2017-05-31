#include <openssl/rand.h>
#include <fc/crypto/openssl.hpp>
#include <fc/exception/exception.hpp>
#include <fc/fwd_impl.hpp>


namespace fc {

void rand_bytes(char* buf, int count)
{
  static int init = init_openssl();

  int result = RAND_bytes((unsigned char*)buf, count);
  if (result != 1)
    FC_THROW("Error calling OpenSSL's RAND_bytes(): ${code}", ("code", (uint32_t)ERR_get_error()));
}

void rand_pseudo_bytes(char* buf, int count)
{
  static int init = init_openssl();

  int result = RAND_pseudo_bytes((unsigned char*)buf, count);
  if (result == -1)
    FC_THROW("Error calling OpenSSL's RAND_pseudo_bytes(): ${code}", ("code", (uint32_t)ERR_get_error()));
}

}  // namespace fc
