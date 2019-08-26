## How to solve the error "Failed to lock access to wallet directory; is another keosd running"?

As `cleos` could auto-launches an instance of `keosd`, it is possible to end up with multiple instances of the `keosd` running. That can cause unexpected behaviours or the error message above. 

To fix this issue, you can terminate the running `keosd` and restart `keosd`. The following command will find and terminate all instances of keosd.

```
$ pkill keosd
```