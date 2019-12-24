## How to solve the error "Failed to lock access to wallet directory; is another kapifinyd running"?

Since `clapifiny` may auto-launch an instance of `kapifinyd`, it is possible to end up with multiple instances of `kapifinyd` running. That can cause unexpected behavior or the error message above.

To fix this issue, you can terminate all running `kapifinyd` instances and restart `kapifinyd`. The following command will find and terminate all instances of `kapifinyd` running on the system:

```sh
$ pkill kapifinyd
```
