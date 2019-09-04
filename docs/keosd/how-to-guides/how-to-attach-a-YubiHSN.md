## Goal

Attach a YubiHSM wallet

## Before you begin

* Install the currently supported version of kleos

* Install YubiHSM2 Software Tookit (YubiHSM2 SDK)

* Delete the default AuthKey 

* Create an AuthKey with at least the following Capabilities:

   * sign-ecdsa
   * generate-asymmetric-key

*  Domain and Object


[[warning | Security]]
| It extremately important to remove the default AuthKey and create a new one before procced to store any keys in YubiHSM.

## Steps

* Check the status by visting YubiHSM URL 

   http://YubiHSM_HOST:YubiHSM_PORT/connector/status ((Default HOST and Port: http://127.0.0.1:12345)

* Start `yubihsm` connector

   #### non-default URL is used

   Set the YubiHSM connector URL by setting `--yubihsm-url` option or `yubihsm-url` in the configuration file.

   #### the default connector URL is used



* Unlock YubiHSM wallet using the following option:

   ```bash
   cleos wallet unlock -n YubiHSM --password YOUR_AUTHKEY_PASSWORD
   ```

   Ater the command below, you can use a YubiHSM hard wallet same as a soft wallet.
   
   Beware as a part of security mechanism some wallet operations are not possible when YubiHSM is used

