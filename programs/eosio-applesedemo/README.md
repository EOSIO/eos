EOSIO supports two different elliptic curves for ECDSA -- secp256k1 and secp256r1 (also called prime256v1).
The benefit of the r1 curve is hardware support in many mobile devices such as iPhones or Android devices. This application
demonstrates usage of the r1 curve in a hardware device via the Secure Enclave on recent MacBook Pros with the Touch Bar. An
r1 private key is stored on the Secure Enclave and is not visible to the host. This private key can then be used to sign SHA256
digests (which could be the hash of an EOSIO transaction) via the Secure Enclave. For verification of proper operation,
 the application then allows verification of public key recovery in the EOSIO code base.
 
# Building and Signing
The application `applesedemo` will be built on macOS builds of the EOSIO source tree. The APIs in use require macOS 10.12+
(and ought to function on iOS10+ as well). However, usage of the Secure Enclave requires that the application
be signed with a Mac App Store (MAS) style signature -- older Darwin style signatures are not sufficient. This strong signature
requirement ensures that the application which generated a particular private key is the only application that can use the private key.

Signing `applesedemo` as a MAS style application is clumsy in our case for two reasons; 1) these signatures require
packaging in an "app bundle" which doesn't fit well for command line applications, and 2) the desire to keep all build steps
devoid of XCode usage.

The script `sign.sh` is included which generates an app bundle `applesedemo.app` and signs it. It must be run with the unsigned
`applesedemo` in its current working directory. Three parameters are required:
1. The SHA1 thumbprint of the signing certificate to use
1. The fully formed AppID of the signed application (teamID + bundleID)
1. Path to the provisioning profile used for signing

There is a commented out example in the CMakeLists.txt which shows how the script can be executed post-build automatically.

Once signed, you must execute the `applesedemo` that is bundled inside the app build, i.e. `applesedemo.app/Contents/MacOS/applesedemo`.
There is a crude check to print an error if attempting to execute an unsigned `applesedemo` but it may not catch cases of
an application that is invalidly signed. An application that is invalidly signed and attempts to use the Secure Enclave
can experience some unhelpful error messages.

## Personal Signing
If you have access to the Apple Developer Program it is possible to use the developer portal to download a provisioning
profile and/or create a developer certificate if you do not already have one.

However, even without a Developer account it's possible to use any Apple ID to generate a development certificate
and provisioning profile that can be used locally. Unfortunately, this process has to be done via XCode's automatic
"managed" signing mechanism. You must create a new Cocoa App in XCode, assign a pertinent bundle identifier, and then select
your "Personal Team" for the Team to use. XCode will then contact Apple's servers and generate a proper provisioning profile
and certificate if you do not already have one. You may have to manually look at the profiles created in
`~/Library/MobileDevice/Provisioning\ Profiles/` to find the proper one to use.

Remember that the TeamID plus the Bundle ID that you fill out in XCode gate access to the Secure Enclave. Changing either
of these values will cause the application to be unable to see keys generated with previous values.

# Usage
## Generate Private Key
First, create a private key inside of the secure enclave. This application supports creating the key in a few different manners
(see `--help`) that control the level of security the key is protected with. In this example, the key will be created in a way
that requires usage of a fingerprint when signing a digest (such a signing a transaction).
```
$ applesedemo.app/Contents/MacOS/applesedemo --create-se-touch-only
Successfully created
public_key(PUB_R1_7iKCZrb1JqSjUncCbDGQenzSqyuNmPU8iUA15efNW5KD1iHd9x)
```
The public key for the private key is printed; note it somewhere. It is possible to ask the Secure Enclave to
create multiple private keys and assign labels to them but this application only allows a single key at once.
## Create A Digest and Sign It
EOSIO uses SHA256 digests to sign transactions, create a simple SHA256 digest from a small string:
```
$ echo 'Hello world!' | shasum -a 256
0ba904eae8773b70c75333db4de2f3ac45a8ad4ddba1b242f0b3cfc199391dd8  -
```
Now, ask the Secure Enclave to sign the digest with the private key only it knows about. The signature will not be
generated until a valid fingerprint is used.
```
$ applesedemo.app/Contents/MacOS/applesedemo --sign 0ba904eae8773b70c75333db4de2f3ac45a8ad4ddba1b242f0b3cfc199391dd8
signature(SIG_R1_Jx4sBidhFV6PSvS8hWbG5oh77HKud8xpkoHLvWaZVaBeWttRpyEjaGbPRVEKu3JePTyVjANmP4GKFtG2DAuB4MTVqsdC9W)
```
## Key Recovery
Given the signature and digest, we must be able to correlate that with the public key that signed the digest.
```
$ applesedemo.app/Contents/MacOS/applesedemo --recover 0ba904eae8773b70c75333db4de2f3ac45a8ad4ddba1b242f0b3cfc199391dd8 \
     SIG_R1_Jx4sBidhFV6PSvS8hWbG5oh77HKud8xpkoHLvWaZVaBeWttRpyEjaGbPRVEKu3JePTyVjANmP4GKFtG2DAuB4MTVqsdC9W
public_key(PUB_R1_7iKCZrb1JqSjUncCbDGQenzSqyuNmPU8iUA15efNW5KD1iHd9x)
```
Indeed, the public key recovered from this digest and signature matches the public key from the key stored in the Secure
Enclave.