### How does `kapifinyd` Locking/Unlocking works and what are the security implications?

From a user's perspective, when a wallet is created, it remains in an `unlocked` state. Depending on the way `kapifinyd` is launched, it may remain `unlocked` until the process is restarted. When a wallet is locked (either by timeout or process restart) the password is required to unlock it.

However, it must be emphasized that `kapifinyd` has no authentication/authorization mechanism besides locking/unlocking the wallet for storing/retrieving private keys and signing digital messages.

### How is the `kapifinyd` service accessed and what are the security implications?

When a domain socket is used to access `kapifinyd`, any UNIX user/group that has access rights to write to the socket file on the filesystem can submit transactions and receive signed transactions from `kapifinyd` using any key in any unlocked wallet.

In the case of a TCP socket bound to localhost, any local process (regardless of owner or permission) can do the same things mentioned above. That includes a snippet of JavaScript code in a web page running in a local browser (though some browsers may have some security mitigations for this).

In the case of a TCP socket bound to a LAN/WAN address, any remote actor that can send packets to a machine running `kapifinyd` may do the same. That present a huge security risk, even if the communication is encrypted or secured via HTTPS.
