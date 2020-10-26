/*
 * Copyright 2015-2018 Yubico AB
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string.h>

#include <arpa/inet.h>

extern "C" {
#include "yubihsm.h"
#include "internal.h"
#include "debug_lib.h"
}

#include <boost/beast.hpp>
#include <boost/asio.hpp>

#undef D
#include <fc/network/url.hpp>

struct state {
  boost::asio::io_context io_context;
  boost::asio::ip::tcp::socket socket{io_context};
  int timeout;
  std::string host;
  std::string port;
  std::string hostport;
  std::string api_path;
};

static void backend_set_verbosity(uint8_t verbosity, FILE *output) {}

static yh_rc backend_init(uint8_t verbosity, FILE *output) {
  return YHR_SUCCESS;
}

static yh_backend *backend_create() {
  state *s = new (std::nothrow) state;
  if(!s)
    return s;
  boost::asio::socket_base::keep_alive ka(true);
  boost::system::error_code ec;
  s->socket.set_option(ka, ec);
  if(!ec) {
    delete s;
    return NULL;
  }
  return (yh_backend*)s;
}

static yh_rc do_connect(state *const s) {
  s->io_context.restart();

  boost::asio::ip::tcp::resolver resolver(s->io_context);
  boost::system::error_code final_ec = boost::asio::error::would_block;
  resolver.async_resolve(s->host, s->port, [&final_ec,&socket=s->socket](auto ec, auto results) {
    if(ec) {
      final_ec = ec;
      return;
    }
    boost::asio::async_connect(socket, results, [&final_ec](auto ec, auto endpoint) {
      final_ec = ec;
    });
  });

  s->io_context.run_for(std::chrono::seconds(s->timeout));

  if(final_ec) {
    s->socket.close();
    return YHR_CONNECTOR_NOT_FOUND;
  }

  return YHR_SUCCESS;
}

static yh_rc backend_connect(yh_connector *connector, int timeout) {
  state *s = reinterpret_cast<state*>(connector->connection);
  s->timeout = timeout ?: INT_MAX;

  std::string path;

  try {
    fc::url hsmurl(connector->status_url);
    if(hsmurl.proto() != "http" || !hsmurl.host() || !hsmurl.path())
      return YHR_CONNECTOR_NOT_FOUND;
    s->host = *hsmurl.host();
    s->port = hsmurl.port() ? std::to_string(*hsmurl.port()) : "80";
    std::stringstream ss;
    ss << s->host << ":" << s->port;
    s->hostport = ss.str();
    path = hsmurl.path()->string();

    fc::url apiurl(connector->api_url);
    if(apiurl.proto() != "http" || !apiurl.host() || !apiurl.path() || apiurl.host() != hsmurl.host())
      return YHR_CONNECTOR_NOT_FOUND;
    s->api_path = apiurl.path()->string();
  }
  catch(...) {
    return YHR_CONNECTOR_NOT_FOUND;
  }

  if(yh_rc yrc = do_connect(s))
    return yrc;

  boost::beast::http::request<boost::beast::http::string_body> req{boost::beast::http::verb::get, path, 11};
  req.set(boost::beast::http::field::host, s->hostport);
  req.set(boost::beast::http::field::user_agent, "yubihsm_wallet");

  boost::system::error_code ec;

  boost::beast::http::write(s->socket, req, ec);
  if(ec) {
    s->socket.close();
    return YHR_CONNECTOR_NOT_FOUND;
  }

  boost::beast::flat_buffer buffer;
  boost::beast::http::response<boost::beast::http::buffer_body> res;
  char buff[256];
  res.body().data = buff;
  res.body().size = sizeof(buff);
  res.body().more = false;

  boost::beast::http::read(s->socket, buffer, res, ec);
  if(ec || res.result() != boost::beast::http::status::ok) {
    s->socket.close();
    return YHR_CONNECTOR_NOT_FOUND;
  }

  return YHR_SUCCESS;
}

static void backend_disconnect(yh_backend *connection) {
  boost::system::error_code ec;
  connection->socket.close(ec);
  delete connection;
}

static yh_rc backend_send_msg(yh_backend *connection, Msg *msg, Msg *response) {
  boost::system::error_code ec;

  boost::beast::http::request<boost::beast::http::buffer_body> req{boost::beast::http::verb::post, connection->api_path, 11};
  req.set(boost::beast::http::field::host, connection->hostport);
  req.set(boost::beast::http::field::content_type, "application/octet-stream");
  req.body().data = msg->raw;
  req.body().size = msg->st.len + 3;
  req.body().more = false;
  auto orig_body = req.body();
  req.prepare_payload();

  msg->st.len = htons(msg->st.len);

  auto cleanup_and_return_on_error = [&connection, &msg, &response]() {
    boost::system::error_code ec;
    connection->socket.close(ec);
    msg->st.len = ntohs(msg->st.len);
    response->st.len = 0;
    return YHR_CONNECTION_ERROR;
  };

  boost::beast::http::write(connection->socket, req, ec);
  if(ec) {
    //try reconnecting once
    if(yh_rc yrc = do_connect(connection))
      return yrc;
    req.body() = orig_body;
    boost::beast::http::write(connection->socket, req, ec);
    if(ec)
      return cleanup_and_return_on_error();
  }

  boost::beast::flat_buffer buffer;
  boost::beast::http::response<boost::beast::http::buffer_body> res;
  res.body().data = response->raw;
  res.body().size = sizeof(response->raw);
  res.body().more = false;
  boost::beast::http::read(connection->socket, buffer, res, ec);
  if(ec || res.result() != boost::beast::http::status::ok)
    return cleanup_and_return_on_error();

  size_t transferred = (uint8_t*)res.body().data - response->raw;

  if(transferred < 3)
    return YHR_WRONG_LENGTH;
  response->st.len = ntohs(response->st.len);
  if(transferred - 3 != response->st.len)
    return YHR_WRONG_LENGTH;

  return YHR_SUCCESS;
}

static void backend_cleanup(void) {}

static yh_rc backend_option(yh_backend *connection, yh_connector_option opt,
                            const void *val) {
  return YHR_CONNECTOR_ERROR;
}

static struct backend_functions f = {backend_init,     backend_create,
                                     backend_connect,  backend_disconnect,
                                     backend_send_msg, backend_cleanup,
                                     backend_option,   backend_set_verbosity};

#ifdef STATIC
struct backend_functions *http_backend_functions(void) {
#else
struct backend_functions *backend_functions(void) {
#endif
  return &f;
}
