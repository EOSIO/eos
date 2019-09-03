## Goal

How to attach a YubiHSM

## Before you begin

* Install the currently supported version of kleos

* Install YubiHSM2 Software Tookit (YubiHSM2 SDK)

* Configure YubiHSM

## Steps

* Check the status by visting YubiHSM URL 

   http://YubiHSM_HOST:YubiHSM_PORT/connector/status ((Default HOST and Port: http://127.0.0.1:12345)

* Create an AuthKey, Domain and Object

### if non-default URL is used

Set the YubiHSM connector URL by setting `--yubihsm-url` option or `yubihsm-url` in the configuration file.

### if the default connector URL is used


