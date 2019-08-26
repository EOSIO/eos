## Failed to lock access to wallet directory; is another keosd running?

Because `cleos` could auto-launches `keosd`, it is possible to end up with multiple instances of the `keosd` running which can cause unexpected behaviour. To fix this issue, you can terminate the running keosds. The following command will find and terminate all instances of keosd.

```
$ pkill keosd
```