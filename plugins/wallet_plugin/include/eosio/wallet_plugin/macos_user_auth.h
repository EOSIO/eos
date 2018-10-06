#pragma once

#include <CoreFoundation/CoreFoundation.h>

//ask for user authentication and call callback with true/false once compelte. **Note that the callback
// will be done in a separate thread**
extern "C" void macos_user_auth(void(*cb)(int, void*), void* cb_userdata, CFStringRef message);