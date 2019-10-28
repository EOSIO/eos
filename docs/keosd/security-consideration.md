### How does `keosd` Locking/Unlocking works and what is the security implication?

From a user's perspective, when a wallet is created, it remains the state of `unlocked`. Depending on the way keosd is launched it may remain `unlocked` until the process is restarted. When a wallet is locked (either by timeout or process restart) the password is required to unlock it.

However, it is critical to know that keosd has no Authentication/Authorization outside of locking/unlocking beside retrieving private keys.

When a domain socket is used for access `keosd`, any UNIX user/group that has access and rights to write to the socket file on the filesystem can submit transactions and receive signed transactions from `keosd` using any key in any unlocked wallet.

In the case of a TCP socket bound to localhost, any local process (regardless of owner or permission) can do the same things mentioned above. That includes a snippet of JavaScript code in a web page running in a local browser (though some browsers may have some mitigations for this)

In the case of a TCP socket bound to a LAN/WAN address, this means any remote actor that can send packets to your machine may do the same and that (even if it is HTTPS and encrypted) presents a huge security risk.
