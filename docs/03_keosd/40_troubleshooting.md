---
content_title: Keosd Troubleshooting
---

## How to solve the error "Failed to lock access to wallet directory; is another `keosd` running"?

Since `cleos` may auto-launch an instance of `keosd`, it is possible to end up with multiple instances of `keosd` running. That can cause unexpected behavior or the error message above.

To fix this issue, you can terminate all running `keosd` instances and restart `keosd`. The following command will find and terminate all instances of `keosd` running on the system:

```sh
pkill keosd
```
