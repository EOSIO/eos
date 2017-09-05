#!/bin/bash

error()
{
  echo $1
  kill -9
  cleanup
  exit 1
}

cleanup()
{
  programs/launcher/launcher -k 9
  kill -9 $WALLETD_PROC_ID
  rm -rf tn_data_0
  rm -rf tn_wallet_0
}

INITA_PRV_KEY="5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"

# eosd
programs/launcher/launcher -p 1
sleep 9
count=`grep -c "generated block" tn_data_0/stderr.txt`
if [ $count == 0 ]; then
  error "FAILURE - no blocks produced"
fi

# create 2 keys
KEYS="$(programs/eosc/eosc create key)"
PRV_KEY1="$(echo "$KEYS" | awk '/Private/ {print $3}')"
PUB_KEY1="$(echo "$KEYS" | awk '/Public/ {print $3}')"
KEYS="$(programs/eosc/eosc create key)"
PRV_KEY2="$(echo "$KEYS" | awk '/Private/ {print $3}')"
PUB_KEY2="$(echo "$KEYS" | awk '/Public/ {print $3}')"
if [ -z "$PRV_KEY1" ] || [ -z "$PRV_KEY2" ] || [ -z "$PUB_KEY1" ] || [ -z "$PUB_KEY2" ]; then
  error "FAILURE - create keys"
fi

# walletd
programs/eos-walletd/eos-walletd --data-dir tn_wallet_0 --http-server-endpoint=127.0.0.1:8899 > test_walletd_output.log 2>&1 &
WALLETD_PROC_ID=$!
sleep 3

# import into a wallet
PASSWORD="$(programs/eosc/eosc --wallet-port 8899 wallet create --name test)"
programs/eosc/eosc --wallet-port 8899 wallet import --name test $PRV_KEY1
programs/eosc/eosc --wallet-port 8899 wallet import --name test $PRV_KEY2
programs/eosc/eosc --wallet-port 8899 wallet import --name test $INITA_PRV_KEY

# create new account
programs/eosc/eosc --wallet-port 8899 create account inita tester $PUB_KEY1 $PUB_KEY2

# verify account created
ACCOUNT_INFO="$(programs/eosc/eosc --wallet-port 8899 get account tester)"
count=`echo $ACCOUNT_INFO | grep -c "exception"`
if [ $count != 0 ]; then
  error "FAILURE - account creation caused exception: $ACCOUNT_INFO"
fi
count=`echo $ACCOUNT_INFO | grep -c "staked_balance"`
if [ $count == 0 ]; then
  error "FAILURE - account creation failed: $ACCOUNT_INFO"
fi

# transfer
programs/eosc/eosc --wallet-port 8899 transfer inita tester 975321 "test transfer"

# verify transfer
ACCOUNT_INFO="$(programs/eosc/eosc --wallet-port 8899 get account tester)"
count=`echo $ACCOUNT_INFO | grep -c "exception"`
if [ $count != 0 ]; then
  error "FAILURE - transfer caused exception: $ACCOUNT_INFO"
fi
count=`echo $ACCOUNT_INFO | grep -c "97.5321"`
if [ $count == 0 ]; then
  error "FAILURE - transfer failed: $ACCOUNT_INFO"
fi

cleanup
echo SUCCESS!
