#import <LocalAuthentication/LocalAuthentication.h>

void macos_user_auth(void(*cb)(int, void*), void* cb_userdata, CFStringRef message) {
   static LAContext* ctx;
   if(ctx)
      [ctx dealloc];
   ctx = [[LAContext alloc] init];
   [ctx evaluatePolicy:kLAPolicyDeviceOwnerAuthentication localizedReason:(NSString*)message reply:^(BOOL success, NSError* error) {
        cb(success, cb_userdata);
    }];
}