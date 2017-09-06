#!/bin/bash

error()
{
  (>&2 echo $1)
  killAll
  exit 1
}

verifyErrorCode()
{
  rc=$?
  if [[ $rc != 0 ]]; then
    error "$1 returned error code $rc"
  fi
}

killAll()
{
  programs/launcher/launcher -k 9
  kill -9 $WALLETD_PROC_ID
}

cleanup()
{
  rm -rf tn_data_0
  rm -rf test_wallet_0
}

# result stored in HEAD_BLOCK_NUM
getHeadBlockNum()
{
  INFO="$(programs/eosc/eosc get info)"
  verifyErrorCode "eosc get info"
  HEAD_BLOCK_NUM="$(echo "$INFO" | awk '/head_block_num/ {print $2}')"
  # remove trailing coma
  HEAD_BLOCK_NUM=${HEAD_BLOCK_NUM%,}
}

waitForNextBlock()
{
  getHeadBlockNum
  NEXT_BLOCK_NUM=$((HEAD_BLOCK_NUM+1))
  while [ "$HEAD_BLOCK_NUM" -lt "$NEXT_BLOCK_NUM" ]; do
    sleep 0.25
    getHeadBlockNum
  done
}

# cleanup from last run
cleanup

INITA_PRV_KEY="5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"
INITB_PRV_KEY="5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"
LOG_FILE=eosd_run_test.log

# eosd
programs/launcher/launcher
verifyErrorCode "launcher"
sleep 9
count=`grep -c "generated block" tn_data_0/stderr.txt`
if [[ $count == 0 ]]; then
  error "FAILURE - no blocks produced"
fi

# save starting block number
getHeadBlockNum
START_BLOCK_NUM=$HEAD_BLOCK_NUM

# create 2 keys
KEYS="$(programs/eosc/eosc create key)"
verifyErrorCode "eosc create key"
PRV_KEY1="$(echo "$KEYS" | awk '/Private/ {print $3}')"
PUB_KEY1="$(echo "$KEYS" | awk '/Public/ {print $3}')"
KEYS="$(programs/eosc/eosc create key)"
verifyErrorCode "eosc create key"
PRV_KEY2="$(echo "$KEYS" | awk '/Private/ {print $3}')"
PUB_KEY2="$(echo "$KEYS" | awk '/Public/ {print $3}')"
if [ -z "$PRV_KEY1" ] || [ -z "$PRV_KEY2" ] || [ -z "$PUB_KEY1" ] || [ -z "$PUB_KEY2" ]; then
  error "FAILURE - create keys"
fi

# walletd
programs/eos-walletd/eos-walletd --data-dir test_wallet_0 --http-server-endpoint=127.0.0.1:8899 > test_walletd_output.log 2>&1 &
verifyErrorCode "eos-walletd"
WALLETD_PROC_ID=$!
sleep 3

# import into a wallet
PASSWORD="$(programs/eosc/eosc --wallet-port 8899 wallet create --name test)"
verifyErrorCode "eosc wallet create"
# strip out password from output
PASSWORD="$(echo "$PASSWORD" | awk '/PW/ {print $1}')"
# remove leading/trailing quotes
PASSWORD=${PASSWORD#\"}
PASSWORD=${PASSWORD%\"}
programs/eosc/eosc --wallet-port 8899 wallet import --name test $PRV_KEY1
verifyErrorCode "eosc wallet import"
programs/eosc/eosc --wallet-port 8899 wallet import --name test $PRV_KEY2
verifyErrorCode "eosc wallet import"

# create wallet for inita
PASSWORD_INITA="$(programs/eosc/eosc --wallet-port 8899 wallet create --name inita)"
verifyErrorCode "eosc wallet create"
# strip out password from output
PASSWORD_INITA="$(echo "$PASSWORD_INITA" | awk '/PW/ {print $1}')"
# remove leading/trailing quotes
PASSWORD_INITA=${PASSWORD_INITA#\"}
PASSWORD_INITA=${PASSWORD_INITA%\"}
programs/eosc/eosc --wallet-port 8899 wallet import --name inita $INITA_PRV_KEY
verifyErrorCode "eosc wallet import"

# lock wallet
programs/eosc/eosc --wallet-port 8899 wallet lock --name test
verifyErrorCode "eosc wallet lock"

# unlock wallet
echo $PASSWORD | programs/eosc/eosc --wallet-port 8899 wallet unlock --name test
verifyErrorCode "eosc wallet unlock"

# lock via lock_all
programs/eosc/eosc --wallet-port 8899 wallet lock_all
verifyErrorCode "eosc wallet lock_all"

# unlock wallet again
echo $PASSWORD | programs/eosc/eosc --wallet-port 8899 wallet unlock --name test
verifyErrorCode "eosc wallet unlock"

# wallet list
OUTPUT=$(programs/eosc/eosc --wallet-port 8899 wallet list)
verifyErrorCode "eosc wallet list"
count=`echo $OUTPUT | grep "test" | grep -c "\*"`
if [[ $count == 0 ]]; then
  error "FAILURE - wallet list did not include \*"
fi

# wallet keys
OUTPUT=$(programs/eosc/eosc --wallet-port 8899 wallet keys)
verifyErrorCode "eosc wallet keys"
count=`echo $OUTPUT | grep -c "$PRV_KEY1"`
if [[ $count == 0 ]]; then
  error "FAILURE - wallet keys did not include $PRV_KEY1"
fi
count=`echo $OUTPUT | grep -c "$PRV_KEY2"`
if [[ $count == 0 ]]; then
  error "FAILURE - wallet keys did not include $PRV_KEY2"
fi

# lock via lock_all
programs/eosc/eosc --wallet-port 8899 wallet lock_all
verifyErrorCode "eosc wallet lock_all"

# unlock wallet inita
echo $PASSWORD_INITA | programs/eosc/eosc --wallet-port 8899 wallet unlock --name inita
verifyErrorCode "eosc wallet unlock inita"
OUTPUT=$(programs/eosc/eosc --wallet-port 8899 wallet keys)
count=`echo $OUTPUT | grep -c "$INITA_PRV_KEY"`
if [[ $count == 0 ]]; then
  error "FAILURE - wallet keys did not include $INITA_PRV_KEY"
fi

# create new account
programs/eosc/eosc --wallet-port 8899 create account inita testera $PUB_KEY1 $PUB_KEY2
verifyErrorCode "eosc create account"
waitForNextBlock

# verify account created
ACCOUNT_INFO="$(programs/eosc/eosc --wallet-port 8899 get account testera)"
verifyErrorCode "eosc get account"
count=`echo $ACCOUNT_INFO | grep -c "staked_balance"`
if [ $count == 0 ]; then
  error "FAILURE - account creation failed: $ACCOUNT_INFO"
fi

# transfer
programs/eosc/eosc --wallet-port 8899 transfer inita testera 975321 "test transfer"
verifyErrorCode "eosc transfer"

# verify transfer
ACCOUNT_INFO="$(programs/eosc/eosc --wallet-port 8899 get account testera)"
count=`echo $ACCOUNT_INFO | grep -c "97.5321"`
if [ $count == 0 ]; then
  error "FAILURE - transfer failed: $ACCOUNT_INFO"
fi

# create another new account via initb
programs/eosc/eosc --wallet-port 8899 create account initb testerb $PUB_KEY1 $PUB_KEY2
verifyErrorCode "eosc create account"
waitForNextBlock

#
# now transfer from testera to testerb using keys from testera
#

# lock via lock_all
programs/eosc/eosc --wallet-port 8899 wallet lock_all
verifyErrorCode "eosc wallet lock_all"

# unlock wallet
echo $PASSWORD | programs/eosc/eosc --wallet-port 8899 wallet unlock --name test
verifyErrorCode "eosc wallet unlock"

# transfer
programs/eosc/eosc --wallet-port 8899 transfer testera testerb 975311 "test transfer"
verifyErrorCode "eosc transfer"

# verify transfer
ACCOUNT_INFO="$(programs/eosc/eosc --wallet-port 8899 get account testerb)"
count=`echo $ACCOUNT_INFO | grep -c "97.5311"`
if [ $count == 0 ]; then
  error "FAILURE - transfer failed: $ACCOUNT_INFO"
fi


# should be able to get every block from beginning to end
getHeadBlockNum
CURRENT_BLOCK_NUM=$HEAD_BLOCK_NUM
NEXT_BLOCK_NUM=1
while [ "$NEXT_BLOCK_NUM" -le "$HEAD_BLOCK_NUM" ]; do
  INFO="$(programs/eosc/eosc get block $NEXT_BLOCK_NUM)"
  verifyErrorCode "eosc get block"
  NEXT_BLOCK_NUM=$((NEXT_BLOCK_NUM+1))
done

killAll
cleanup
echo SUCCESS!
