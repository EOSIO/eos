#include <eosiolib/types.h>
#include "vm_api.h"
extern "C" void vm_api_throw_exception(int type, const char* fmt, ...) {
    get_vm_api()->throw_exception(type, fmt);
}
