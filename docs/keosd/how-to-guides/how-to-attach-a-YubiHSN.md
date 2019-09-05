## Goal

Attach a YubiHSM as a hard wallet

[[warning | Security]]
| It is extremately important to remove the default AuthKey and create a new AuthKey before procced to connect `keosd` to YubiHSM.

## Before you begin

* Install the currently supported version of kleos

* Install YubiHSM2 Software Tookit (YubiHSM2 SDK)

* **Delete the default AuthKey**

* Create an AuthKey with at least the following Capabilities:

   * sign-ecdsa
   * generate-asymmetric-key

## Steps

* Check the status by visiting YubiHSM URL

   http://YubiHSM_HOST:YubiHSM_PORT/connector/status ((Default HOST and Port: http://127.0.0.1:12345)

* Start `yubihsm` connector

   There are two options to connect `keosd` to YubiHSM

   #### Using a YubiHSM connector

   By default, `keosd` will connect to the YubiHSM connector on the default host and port

   if a different URL is used, set the YubiHSM connector URL by setting `--yubihsm-url` option or `yubihsm-url` in `config.ini`

   #### Directly connect via USB

   `keosd` also can directly connect to YubiHSM via USB protocol

   If this option is used, set `keosd` startup option as the below:

   ```bash
   --yubihsm-url=ysb://
   ```

* Unlock YubiHSM wallet with the password of AuthKey using the following option:

   ```bash
   cleos wallet unlock -n YubiHSM --password YOUR_AUTHKEY_PASSWORD
   ```

   Ater the command below, you can use a YubiHSM hard wallet same as a soft wallet
   
   Beware as a part of security mechanism some wallet operations are not possible when YubiHSM is used
