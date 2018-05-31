/**
 *  @file
 *  @copyright defined in LICENSE.txt
 */
#pragma once

#include <fc/static_variant.hpp>
#include <fc/time.hpp>
#include <fc/variant.hpp>
#include <fc/exception/exception.hpp>
#include <fc/network/url.hpp>

namespace fc {

class http_client {
   public:
      http_client();
      ~http_client();

      variant post_sync(const url& dest, const variant& payload);

      template<typename T>
      variant post_sync(const url& dest, const T& payload) {
         variant payload_v;
         to_variant(payload, payload_v);
         return post_sync(dest, payload_v);
      }

      void set_timeout(microseconds timeout);

      void add_cert(const std::string& cert_pem_string);

private:
   std::unique_ptr<class http_client_impl> _my;
};

}