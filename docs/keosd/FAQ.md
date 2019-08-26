### How does keosd store key pair

`keosd` encrypts key pairs underneath before storing them to a wallet file. Depends on implementation of wallet medium such as Secure Clave or YubiHSM, a specific cryptography algorithms will be used. When a standard file system of OS is used, `keosd` encrypt key pairs using 256bit AES in CBC mode.

### How to enable Secure Enclave

To enable the secure enclave feature of `keosd`, you need to sign a `keosd` binary with a certificate provided with your Apple Developer Account. Be aware that there are some constraint on signning a console application at moment imposed by App Store, therefore, the signed binaries need to be resign every 7 days.
