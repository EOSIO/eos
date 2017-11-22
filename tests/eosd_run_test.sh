#!/bin/bash

# prevent bc from adding \ at end of large hex values
export BC_LINE_LENGTH=9999

TEST_OUTPUT="test_output_0.txt"

if [ $# -lt 2 ]; then
  echo "usage: eosd_run_test.sh --host=host_name --port=port --output=test_output_0.txt"
  echo "  If --host==localhost then this script also launches an eosd for the test."
  echo "  Example: eosd_run_test_sh --host=localhost --port=8888 --output=test_output_0.txt"
  exit 1
fi

for i in "$@"
do
case $i in
	-o*|--output=*)
	TEST_OUTPUT="${i#*=}"
	shift
	;;
        -h*|--host=*)
	SERVER="${i#*=}"
	shift
	;;
        -p*|--port=*)
	PORT="${i#*=}"
	shift
	;;
        *)
	;;
esac
done


# $1 - error message
error()
{
  (>&2 echo $1)
  killAll
  echo =================================================================
  echo Contents of tn_data_0/config.ini:
  cat tn_data_0/config.ini
  echo =================================================================
  echo Contents of tn_data_0/stderr.txt:
  cat tn_data_0/stderr.txt
  echo =================================================================
  echo Contents of test_walletd_output.log:
  cat test_walletd_output.log
  echo =================================================================
  echo == Errors see above ==
  exit 1
}

verifyErrorCode()
{
  rc=$?
  if [[ $rc != 0 ]]; then
    error "FAILURE - $1 returned error code $rc"
  fi
}

killAll()
{
  if [ "$SERVER" == "localhost" ]; then
    programs/launcher/launcher -k 9
  fi
  kill -9 $WALLETD_PROC_ID
}

cleanup()
{
  rm -rf tn_data_0
  rm -rf test_wallet_0
}

# $1 - string that contains "transaction_id": "<trans id>", in it
# result <trans id> stored in TRANS_ID
getTransactionId()
{
  TRANS_ID="$(echo "$1" | awk '/transaction_id/ {print $2}')"
  # remove leading/trailing quotes
  TRANS_ID=${TRANS_ID#\"}
  TRANS_ID=${TRANS_ID%\",}
}

# result stored in HEAD_BLOCK_NUM
getHeadBlockNum()
{
  INFO="$(programs/eosc/eosc --host $SERVER --port $PORT get info)"
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

echo "BEGIN" > $TEST_OUTPUT
echo "TEST_OUTPUT: ${TEST_OUTPUT}"
echo "SERVER: ${SERVER}"
echo "PORT: ${PORT}"

INITA_PRV_KEY="5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"
INITB_PRV_KEY="5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"
LOG_FILE=eosd_run_test.log

# eosd
if [ "$SERVER" == "localhost" ]; then
  programs/launcher/launcher
  verifyErrorCode "launcher"
  sleep 7
  count=`grep -c "generated block" tn_data_0/stderr.txt`
  if [[ $count == 0 ]]; then
    error "FAILURE - no blocks produced"
  fi
fi

# save starting block number
getHeadBlockNum
START_BLOCK_NUM=$HEAD_BLOCK_NUM

# create 3 keys
KEYS="$(programs/eosc/eosc create key)"
verifyErrorCode "eosc create key"
PRV_KEY1="$(echo "$KEYS" | awk '/Private/ {print $3}')"
PUB_KEY1="$(echo "$KEYS" | awk '/Public/ {print $3}')"
KEYS="$(programs/eosc/eosc create key)"
verifyErrorCode "eosc create key"
PRV_KEY2="$(echo "$KEYS" | awk '/Private/ {print $3}')"
PUB_KEY2="$(echo "$KEYS" | awk '/Public/ {print $3}')"
KEYS="$(programs/eosc/eosc create key)"
verifyErrorCode "eosc create key"
PRV_KEY3="$(echo "$KEYS" | awk '/Private/ {print $3}')"
PUB_KEY3="$(echo "$KEYS" | awk '/Public/ {print $3}')"
if [ -z "$PRV_KEY1" ] || [ -z "$PRV_KEY2" ] || [ -z "$PRV_KEY3" ] || [ -z "$PUB_KEY1" ] || [ -z "$PUB_KEY2" ] || [ -z "$PUB_KEY3" ]; then
  error "FAILURE - create keys"
fi

#
# Wallet Tests
#

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
programs/eosc/eosc --wallet-port 8899 wallet import --name test $PRV_KEY3
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

#
# Account and Transfer Tests
#

# create new account
ACCOUNT_INFO="$(programs/eosc/eosc --host $SERVER --port $PORT --wallet-port 8899 create account inita testera $PUB_KEY1 $PUB_KEY3)"
verifyErrorCode "eosc create account"
waitForNextBlock

# verify account created
ACCOUNT_INFO="$(programs/eosc/eosc --host $SERVER --port $PORT --wallet-port 8899 get account testera)"
verifyErrorCode "eosc get account"
count=`echo $ACCOUNT_INFO | grep -c "staked_balance"`
if [ $count == 0 ]; then
  error "FAILURE - account creation failed: $ACCOUNT_INFO"
fi

# transfer
TRANSFER_INFO="$(programs/eosc/eosc --host $SERVER --port $PORT --wallet-port 8899 transfer inita testera 975321 "test transfer")  -x 3000"
verifyErrorCode "eosc transfer"

# verify transfer
ACCOUNT_INFO="$(programs/eosc/eosc --host $SERVER --port $PORT --wallet-port 8899 get account testera)"
count=`echo $ACCOUNT_INFO | grep -c "97.5321"`
if [ $count == 0 ]; then
  error "FAILURE - transfer failed: $ACCOUNT_INFO"
fi

# force transfer
TRANSFER_INFO="$(programs/eosc/eosc --host $SERVER --port $PORT --wallet-port 8899 transfer inita testera 100 "test transfer" -f)  -x 3000"
verifyErrorCode "eosc force transfer"

# verify force transfer
ACCOUNT_INFO="$(programs/eosc/eosc --host $SERVER --port $PORT --wallet-port 8899 get account testera)"
count=`echo $ACCOUNT_INFO | grep -c "97.5421"`
if [ $count == 0 ]; then
  error "FAILURE - transfer failed: $ACCOUNT_INFO"
fi


# create another new account via initb
ACCOUNT_INFO="$(programs/eosc/eosc --host $SERVER --port $PORT --wallet-port 8899 create account initb currency $PUB_KEY2 $PUB_KEY3)"
verifyErrorCode "eosc create account"
waitForNextBlock

## now transfer from testera to currency using keys from testera

# lock via lock_all
programs/eosc/eosc --wallet-port 8899 wallet lock_all
verifyErrorCode "eosc wallet lock_all"

# unlock wallet
echo $PASSWORD | programs/eosc/eosc --wallet-port 8899 wallet unlock --name test
verifyErrorCode "eosc wallet unlock"

# transfer
TRANSFER_INFO="$(programs/eosc/eosc --host $SERVER --port $PORT --wallet-port 8899 transfer testera currency 975311 "test transfer a->b")  -x 3000"
verifyErrorCode "eosc transfer"
waitForNextBlock
getTransactionId "$TRANSFER_INFO"

# verify transfer
ACCOUNT_INFO="$(programs/eosc/eosc --host $SERVER --port $PORT --wallet-port 8899 get account currency)"
verifyErrorCode "eosc get account currency"
count=`echo $ACCOUNT_INFO | grep -c "97.5311"`
if [ $count == 0 ]; then
  error "FAILURE - transfer failed: $ACCOUNT_INFO"
fi

# get accounts via public key
ACCOUNT_INFO="$(programs/eosc/eosc --host $SERVER --port $PORT --wallet-port 8899 get accounts $PUB_KEY3)"
verifyErrorCode "eosc get accounts pub_key3"
count=`echo $ACCOUNT_INFO | grep -c "testera"`
if [ $count == 0 ]; then
  error "FAILURE - get accounts failed: $ACCOUNT_INFO"
fi
count=`echo $ACCOUNT_INFO | grep -c "currency"`
if [ $count == 0 ]; then
  error "FAILURE - get accounts failed: $ACCOUNT_INFO"
fi
ACCOUNT_INFO="$(programs/eosc/eosc --host $SERVER --port $PORT --wallet-port 8899 get accounts $PUB_KEY1)"
verifyErrorCode "eosc get accounts pub_key1"
count=`echo $ACCOUNT_INFO | grep -c "testera"`
if [ $count == 0 ]; then
  error "FAILURE - get accounts failed: $ACCOUNT_INFO"
fi
count=`echo $ACCOUNT_INFO | grep -c "currency"`
if [ $count != 0 ]; then
  error "FAILURE - get accounts failed: $ACCOUNT_INFO"
fi

# get servant accounts
ACCOUNT_INFO="$(programs/eosc/eosc --host $SERVER --port $PORT --wallet-port 8899 get servants inita)"
verifyErrorCode "eosc get servants inita"
count=`echo $ACCOUNT_INFO | grep -c "testera"`
if [ $count == 0 ]; then
  error "FAILURE - get servants failed: $ACCOUNT_INFO"
fi
count=`echo $ACCOUNT_INFO | grep -c "currency"`
if [ $count != 0 ]; then
  error "FAILURE - get servants failed: $ACCOUNT_INFO"
fi
ACCOUNT_INFO="$(programs/eosc/eosc --host $SERVER --port $PORT --wallet-port 8899 get servants testera)"
verifyErrorCode "eosc get servants testera"
count=`echo $ACCOUNT_INFO | grep -c "testera"`
if [ $count != 0 ]; then
  error "FAILURE - get servants failed: $ACCOUNT_INFO"
fi

# get transaction
TRANS_INFO="$(programs/eosc/eosc --host $SERVER --port $PORT --wallet-port 8899 get transaction $TRANS_ID)"
verifyErrorCode "eosc get transaction trans_id of $TRANS_INFO"
count=`echo $TRANS_INFO | grep -c "transfer"`
if [ $count == 0 ]; then
  error "FAILURE - get transaction trans_id failed: $TRANS_INFO"
fi
count=`echo $TRANS_INFO | grep -c "975311"`
if [ $count == 0 ]; then
  error "FAILURE - get transaction trans_id failed: $TRANS_INFO"
fi

# get transactions
TRANS_INFO="$(programs/eosc/eosc --host $SERVER --port $PORT  --wallet-port 8899 get transactions testera)"
verifyErrorCode "eosc get transactions testera"
count=`echo $TRANS_INFO | grep -c "$TRANS_ID"`
if [ $count == 0 ]; then
  error "FAILURE - get transactions testera failed: $TRANS_INFO"
fi

#
# Currency Contract Tests
#

# verify no contract in place
CODE_INFO="$(programs/eosc/eosc --host $SERVER --port $PORT --wallet-port 8899 get code currency)"
verifyErrorCode "eosc get code currency"
# convert to num
CODE_HASH="$(echo "$CODE_INFO" | awk '/code hash/ {print $3}')"
CODE_HASH="$(echo $CODE_HASH | awk '{print toupper($0)}')"
CODE_HASH="$(echo "ibase=16; $CODE_HASH" | bc)"
if [ $CODE_HASH != 0 ]; then
  error "FAILURE - get code currency failed: $CODE_INFO"
fi

# upload a contract
INFO="$(programs/eosc/eosc --host $SERVER --port $PORT --wallet-port 8899 set contract currency contracts/currency/currency.wast contracts/currency/currency.abi)"
verifyErrorCode "eosc set contract testera"
count=`echo $INFO | grep -c "processed"`
if [ $count == 0 ]; then
  error "FAILURE - set contract failed: $INFO"
fi
getTransactionId "$INFO"

waitForNextBlock
# verify transaction exists
TRANS_INFO="$(programs/eosc/eosc --host $SERVER --port $PORT --wallet-port 8899 get transaction $TRANS_ID)"
verifyErrorCode "eosc get transaction trans_id of $TRANS_INFO"

# verify code is set
CODE_INFO="$(programs/eosc/eosc --host $SERVER --port $PORT --wallet-port 8899 get code currency)"
verifyErrorCode "eosc get code currency"
# convert to num
CODE_HASH="$(echo "$CODE_INFO" | awk '/code hash/ {print $3}')"
CODE_HASH="$(echo $CODE_HASH | awk '{print toupper($0)}')"
CODE_HASH="$(echo "ibase=16; $CODE_HASH" | bc)"
if [ $CODE_HASH == 0 ]; then
  error "FAILURE - get code currency failed: $CODE_INFO"
fi

# verify currency contract has proper initial balance
INFO="$(programs/eosc/eosc --host $SERVER --port $PORT --wallet-port 8899 get table currency currency account)"
verifyErrorCode "eosc get table currency account"
count=`echo $INFO | grep -c "1000000000"`
if [ $count == 0 ]; then
  error "FAILURE - get table currency account failed: $INFO"
fi
count=`echo $INFO | grep -c "account"`
if [ $count == 0 ]; then
  error "FAILURE - get table currency account failed: $INFO"
fi

# push message to currency contract

INFO="$(programs/eosc/eosc --host $SERVER --port $PORT --wallet-port 8899 push message currency transfer '{"from":"currency","to":"inita","quantity":50}' --scope currency,inita --permission currency@active)"
verifyErrorCode "eosc push message currency transfer"
getTransactionId "$INFO"

# verify transaction exists
waitForNextBlock
TRANS_INFO="$(programs/eosc/eosc --host $SERVER --port $PORT --wallet-port 8899 get transaction $TRANS_ID)"
verifyErrorCode "eosc get transaction trans_id of $TRANS_INFO"

# read current contract balance
ACCOUNT_INFO="$(programs/eosc/eosc --host $SERVER --port $PORT --wallet-port 8899 get table inita currency account)"
verifyErrorCode "eosc get table currency account"
count=`echo $ACCOUNT_INFO | grep "balance" | grep -c "50"`
if [ $count == 0 ]; then
  error "FAILURE - get table inita account failed: $ACCOUNT_INFO"
fi

ACCOUNT_INFO="$(programs/eosc/eosc --host $SERVER --port $PORT --wallet-port 8899 get table currency currency account)"
verifyErrorCode "eosc get table currency account"
count=`echo $ACCOUNT_INFO | grep "balance" | grep -c "999999950"`
if [ $count == 0 ]; then
  error "FAILURE - get table currency account failed: $ACCOUNT_INFO"
fi

#
# Producer
#

INFO="$(programs/eosc/eosc --host $SERVER --port $PORT --wallet-port 8899 create producer testera $PUB_KEY1)"
verifyErrorCode "eosc create producer"
getTransactionId "$INFO"

# set permission
INFO="$(programs/eosc/eosc --host $SERVER --port $PORT --wallet-port 8899 set action permission testera currency transfer active)"
verifyErrorCode "eosc set action permission set"
waitForNextBlock

# remove permission
INFO="$(programs/eosc/eosc --host $SERVER --port $PORT --wallet-port 8899 set action permission testera currency transfer null)"
verifyErrorCode "eosc set action permission delete"

# lock via lock_all
programs/eosc/eosc --host $SERVER --port $PORT --wallet-port 8899 wallet lock_all
verifyErrorCode "eosc wallet lock_all"

# unlock wallet inita
echo $PASSWORD_INITA | programs/eosc/eosc --host $SERVER --port $PORT --wallet-port 8899 wallet unlock --name inita
verifyErrorCode "eosc wallet unlock inita"

# approve producer
INFO="$(programs/eosc/eosc --host $SERVER --port $PORT --wallet-port 8899 set producer inita testera approve)"
verifyErrorCode "eosc approve producer"

ACCOUNT_INFO="$(programs/eosc/eosc --host $SERVER --port $PORT --wallet-port 8899 get account inita)"
verifyErrorCode "eosc get account"

# unapprove producer
INFO="$(programs/eosc/eosc --host $SERVER --port $PORT --wallet-port 8899 set producer inita testera unapprove)"
verifyErrorCode "eosc unapprove producer"

#
# Proxy
#
# not implemented

# should be able to get every block from beginning to end
getHeadBlockNum
CURRENT_BLOCK_NUM=$HEAD_BLOCK_NUM
NEXT_BLOCK_NUM=1
while [ "$NEXT_BLOCK_NUM" -le "$HEAD_BLOCK_NUM" ]; do
  INFO="$(programs/eosc/eosc --host $SERVER --port $PORT get block $NEXT_BLOCK_NUM)"
  verifyErrorCode "eosc get block"
  NEXT_BLOCK_NUM=$((NEXT_BLOCK_NUM+1))
done

ASSERT_ERRORS="$(grep Assert tn_data_0/stderr.txt)"
count=`grep -c Assert tn_data_0/stderr.txt`
if [ $count != 0 ]; then
  error "FAILURE - Assert in tn_data_0/stderr.txt"
fi

killAll
cleanup
echo "END" >> $TEST_OUTPUT
echo SUCCESS!

