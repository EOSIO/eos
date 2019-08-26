### How does keosd store key pair

Underneath `keosd` encrypts key pairs before storing them in a wallet file. Depends on implementation of different wallet medium such as Secure Clave, HSM, different cryptography algorithms might be used. When a standard file system of OS is used, `keosd` encrypt key pairs using AES.

### How to enable Secure Enclave

To enable the secure enclave feature of `keosd`, you need to sign a `keosd` binary with a certificate provided with your Apple Developer Account. Be aware that there are some constraint on signning a console application at moment imposed by App Store, therefore, the signed binaries need to be resign every 7 days.