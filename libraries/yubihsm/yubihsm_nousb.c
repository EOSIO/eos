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

#include <stdint.h>
#include <string.h>

#include "yubihsm.h"
#include "internal.h"

static void backend_set_verbosity(uint8_t verbosity, FILE *output) {}

static yh_rc backend_init(uint8_t verbosity, FILE *output) {
  return YHR_CONNECTION_ERROR;
}

yh_backend *backend_create(void) {
  return NULL;
}

static yh_rc backend_connect(yh_connector *connector, int timeout) {
  return YHR_CONNECTION_ERROR;
}

static void backend_disconnect(yh_backend *connection) {}

static yh_rc backend_send_msg(yh_backend *connection, Msg *msg, Msg *response) {
   return YHR_CONNECTION_ERROR;
}

static void backend_cleanup(void) {}

static yh_rc backend_option(yh_backend *connection, yh_connector_option opt,
                            const void *val) {
   return YHR_CONNECTION_ERROR;
}

static struct backend_functions f = {backend_init,     backend_create,
                                     backend_connect,  backend_disconnect,
                                     backend_send_msg, backend_cleanup,
                                     backend_option,   backend_set_verbosity};

#ifdef STATIC
struct backend_functions *usb_backend_functions(void) {
#else
struct backend_functions *backend_functions(void) {
#endif
  return &f;
}
