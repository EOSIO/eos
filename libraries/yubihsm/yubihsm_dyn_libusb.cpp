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

#include <fc/log/logger.hpp>

extern "C" {
#include <libusb.h>

#include <string.h>
#include <dlfcn.h>

#include "yubihsm.h"
#include "internal.h"
#include "yubihsm_usb.h"
#include "debug_lib.h"
}

struct libusb_api {
  struct func_ptr {
     explicit func_ptr(void* ptr) : _ptr(ptr) {}
     template <typename T> operator T*() const {
        return reinterpret_cast<T*>(_ptr);
      }
     void* _ptr;
  };

  struct libusb_shlib {
    libusb_shlib() {
      const char* lib_name =
#if defined( __APPLE__ )
        "libusb-1.0.dylib";
#else
        "libusb-1.0.so";
#endif
      _good = _handle = dlopen(lib_name, RTLD_NOW);
      if(!_good)
        elog("Failed to load libusb: %{e}", ("e", dlerror()));
    }
    ~libusb_shlib() {
      dlclose(_handle);
    }

    func_ptr operator[](const char* import_name) {
      if(!_good)
        return func_ptr(NULL);
      dlerror();
      void* ret = dlsym(_handle, import_name);
      char* error;
      if((error = dlerror())) {
        elog("Failed to load import ${i} from libusb: ${e}", ("i", import_name)("e", error));
        _good = false;
      }
      return func_ptr(ret);
    }

    void* _handle;
    bool _good = false;
  };
  libusb_shlib _shlib;
  bool good = _shlib._good;
   

#define LOAD_IMPORT(n) decltype(::n)* n = _shlib[#n];
  LOAD_IMPORT(libusb_release_interface)
  LOAD_IMPORT(libusb_close)
  LOAD_IMPORT(libusb_exit)
  LOAD_IMPORT(libusb_init)
  LOAD_IMPORT(libusb_get_device_list)
  LOAD_IMPORT(libusb_get_device_descriptor)
  LOAD_IMPORT(libusb_get_string_descriptor_ascii)
  LOAD_IMPORT(libusb_claim_interface)
  LOAD_IMPORT(libusb_free_device_list)
  LOAD_IMPORT(libusb_bulk_transfer)
  LOAD_IMPORT(libusb_error_name)
  LOAD_IMPORT(libusb_open)
};

struct state {
  libusb_context *ctx;
  libusb_device_handle *handle;
  libusb_api *api;
  unsigned long serial;
};

extern "C" {

void usb_set_serial(yh_backend *state, unsigned long serial) {
  state->serial = serial;
}

void usb_close(yh_backend *state) {
  if (state && state->handle) {
    state->api->libusb_release_interface(state->handle, 0);
    state->api->libusb_close(state->handle);
    state->handle = NULL;
  }
}

void usb_destroy(yh_backend **state) {
  if (state && *state) {
    usb_close(*state);
    if ((*state)->ctx) {
      (*state)->api->libusb_exit((*state)->ctx);
      (*state)->ctx = NULL;
    }
    delete (*state)->api;
    free(*state);
    *state = NULL;
  }
}

yh_backend *backend_create(void) {
  yh_backend *backend = (yh_backend*)calloc(1, sizeof(yh_backend));
  if (backend) {
    backend->api = new libusb_api();
    if(backend->api->good == false) {
      delete backend->api;
      free(backend);
      return NULL;
    }
    backend->api->libusb_init(&backend->ctx);
  }
  return backend;
}

bool usb_open_device(yh_backend *backend) {
  libusb_device **list;
  libusb_device_handle *h = NULL;
  ssize_t cnt = backend->api->libusb_get_device_list(backend->ctx, &list);

  if (backend->handle) {
    usb_close(backend);
  }

  if (cnt < 0) {
    DBG_ERR("Failed to get device list: %s", backend->api->libusb_error_name((libusb_error)cnt));
    return false;
  }

  for (ssize_t i = 0; i < cnt; i++) {
    struct libusb_device_descriptor desc;
    int ret = backend->api->libusb_get_device_descriptor(list[i], &desc);
    if (ret != 0) {
      DBG_INFO("Failed to get descriptor for device %zd: %s", i,
               backend->api->libusb_error_name((libusb_error)ret));
      continue;
    }
    if (desc.idVendor == YH_VID && desc.idProduct == YH_PID) {
      ret = backend->api->libusb_open(list[i], &h);
      if (ret != 0 || h == NULL) {
        DBG_INFO("Failed to open device for index %zd: %s", i,
                 backend->api->libusb_error_name((libusb_error)ret));
        continue;
      }
      if (backend->serial != 0) {
        unsigned char data[16] = {0};

        ret = backend->api->libusb_get_string_descriptor_ascii(h, desc.iSerialNumber, data,
                                                 sizeof(data));

        unsigned long devSerial = strtoul((char *) data, NULL, 10);

        if (devSerial != backend->serial) {
          DBG_INFO("Device %zd has serial %lu, not matching searched %lu", i,
                   devSerial, backend->serial);
          goto next;
        }
      }

      ret = backend->api->libusb_claim_interface(h, 0);
      if (ret != 0) {
        DBG_ERR("Failed to claim interface: %s of device %zd",
                backend->api->libusb_error_name((libusb_error)ret), i);
        goto next;
      }

      break;
    next:
      backend->api->libusb_close(h);
      h = NULL;
    }
  }

  backend->api->libusb_free_device_list(list, 1);
  backend->handle = h;
  if (h) {
    // we set up a dummy read with a 1ms timeout here. The reason for doing this
    // is that there might be data left in th e device buffers from earlier
    // transactions, this should flush it.
    unsigned char buf[YH_MSG_BUF_SIZE];
    int transferred = 0;
    if (backend->api->libusb_bulk_transfer(h, 0x81, buf, sizeof(buf), &transferred, 1) == 0) {
      DBG_INFO("%d bytes of stale data read from device", transferred);
    }
    return true;
  } else {
    return false;
  }
}

int usb_write(yh_backend *state, unsigned char *buf, long unsigned len) {
  int transferred = 0;
  if (state->handle == NULL) {
    DBG_ERR("Handle is not connected");
    return 0;
  }
  /* TODO: does this need to loop and transmit several times? */
  int ret =
    state->api->libusb_bulk_transfer(state->handle, 0x01, buf, len, &transferred, 0);
  DBG_INFO("Write of %lu %d, err %d", len, transferred, ret);
  if (ret != 0 || transferred != (int) len) {
    DBG_ERR("Transferred did not match len of write %d-%lu", transferred, len);
    return 0;
  }
  if (len % 64 == 0) {
    /* this writes the ZLP */
    ret = state->api->libusb_bulk_transfer(state->handle, 0x01, buf, 0, &transferred, 0);
    if (ret != 0) {
      return 0;
    }
  }
  return 1;
}

int usb_read(yh_backend *state, unsigned char *buf, unsigned long *len) {
  int transferred = 0;
  int ret;

  if (state->handle == NULL) {
    DBG_ERR("Handle is not connected");
    return 0;
  }

  DBG_INFO("Doing usb read");

  /* TODO: does this need to loop for all data?*/
  ret = state->api->libusb_bulk_transfer(state->handle, 0x81, buf, *len, &transferred, 0);
  if (ret != 0) {
    DBG_ERR("Failed usb_read with ret: %d", ret);
    return 0;
  }
  DBG_INFO("Read, transfer %d", transferred);
  *len = transferred;
  return 1;
}

}