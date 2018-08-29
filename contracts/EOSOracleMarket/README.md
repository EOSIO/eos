# EOSOracleMarket
# EOSOracleMarket
#eosoramar
doushadeMacBook-Pro:EOSOracleMarket cl$ cleos create key
Private key: 5JzmLoEDas93Z9P7j6uh3hfpoSTx3irpdvJuwJRbHucBobmmPb9
Public key: EOS5XmwjhYbxkCmxo4ifHRqtmppWjt46xcEJrs1iK4NbQQNCtDSTG
doushadeMacBook-Pro:EOSOracleMarket cl$ cleos create key
Private key: 5KTWwHj6DNiJ7b1uEhXFCaxYwfLmP6Wyus6btc8JNJhjBvEj1Ej
Public key: EOS7p54umtYDvWWdwbXCr1XeXV2h415uRqiNm2U4FzvsMk2Pfzrxu

cleos -u http://127.0.0.1:8888/ --wallet-url http://127.0.0.1:8890/ create account eosio eosoramar EOS5XmwjhYbxkCmxo4ifHRqtmppWjt46xcEJrs1iK4NbQQNCtDSTG EOS7p54umtYDvWWdwbXCr1XeXV2h415uRqiNm2U4FzvsMk2Pfzrxu

cleos -u http://127.0.0.1:8888/ --wallet-url http://127.0.0.1:8890/ wallet import 5KTWwHj6DNiJ7b1uEhXFCaxYwfLmP6Wyus6btc8JNJhjBvEj1Ej
cleos -u http://127.0.0.1:8888/ --wallet-url http://127.0.0.1:8890/ wallet import 5K7HPhpsf2Q1toX6H6uvWdaURvorZQ7TJJmoHRRJPxF9FFMx74h
cleos -u http://127.0.0.1:8889/ --wallet-url http://127.0.0.1:8890/ set contract eosoramar ./ oraclemarket.wast oraclemarket.abi -p eosoramar

doushadeMacBook-Pro:EOSOracleMarket cl$ cleos create key
Private key: 5K7HPhpsf2Q1toX6H6uvWdaURvorZQ7TJJmoHRRJPxF9FFMx74h
Public key: EOS7k3exA7CzgLgK4g1MK49wy9jozwK8ntuoCsv6oF3feD4C68wCP
cleos -u http://127.0.0.1:8888/ --wallet-url http://127.0.0.1:8890/ set account permission eosoramar active '{"threshold": 1,"keys": [{"key": "EOS7k3exA7CzgLgK4g1MK49wy9jozwK8ntuoCsv6oF3feD4C68wCP","weight": 1}],"accounts": [{"permission":{"actor":"eosoramar","permission":"eosio.code"},"weight":1}]}' owner -p eosoramar

cleos -u http://127.0.0.1:8888/ --wallet-url http://127.0.0.1:8890/ push action octoneos issue '[ "answera", "100.0000 OCT", "m" ]' -p octoneos
cleos -u http://127.0.0.1:8888/ --wallet-url http://127.0.0.1:8890/ push action octoneos issue '[ "answerb", "100.0000 OCT", "m" ]' -p octoneos
cleos -u http://127.0.0.1:8888/ --wallet-url http://127.0.0.1:8890/ push action octoneos approve '{"owner":"answera", "spender":"eosoramar", "quantity":"100.0000 OCT"}' -p answera
cleos -u http://127.0.0.1:8888/ --wallet-url http://127.0.0.1:8890/ push action octoneos approve '{"owner":"answerb", "spender":"eosoramar", "quantity":"100.0000 OCT"}' -p answerb

cleos -u http://127.0.0.1:8888/ --wallet-url http://127.0.0.1:8890/ push action eosoramar mortgage '{"from":"answera", "server":"ocaskans", "quantity":"11.0000 OCT"}' -p answera
cleos -u http://127.0.0.1:8888/ --wallet-url http://127.0.0.1:8890/ push action eosoramar mortgage '{"from":"answerb", "server":"ocaskans", "quantity":"11.0000 OCT"}' -p answerb

cleos -u http://127.0.0.1:8888/ --wallet-url http://127.0.0.1:8890/  get table eosoramar answera mortgaged
cleos -u http://127.0.0.1:8888/ --wallet-url http://127.0.0.1:8890/  get table eosoramar answerb mortgaged

cleos -u http://127.0.0.1:8888/ --wallet-url http://127.0.0.1:8890/ push action eosoramar setconscolim '{"conadm":"ocaskans", "assfrosec":"20", "scores":"0", "fee":"1.0000 OCT"}' -p ocaskans
cleos -u http://127.0.0.1:8888/ --wallet-url http://127.0.0.1:8890/  get table eosoramar ocaskans contractinfo


cleos -u http://127.0.0.1:8888/ --wallet-url http://127.0.0.1:8890/ push action eosoramar votebehavior '{"server":"ocaskans", "user":"answera", "memo":"askid=1"}' -p ocaskans

cleos -u http://127.0.0.1:8888/ --wallet-url http://127.0.0.1:8890/ push action eosoramar withdrawfro '{"from":"answera"}' -p answera
cleos -u http://127.0.0.1:8888/ --wallet-url http://127.0.0.1:8890/ push action eosoramar withdrawfro '{"from":"answerb"}' -p answerb

cleos -u http://127.0.0.1:8888/ --wallet-url http://127.0.0.1:8890/ push action eosoramar clear '{"scope":"eosoramar", "id":"2"}' -p eosoramar


cleos -u http://127.0.0.1:8888/ --wallet-url http://127.0.0.1:8890/ push action eosoramar admincheck '{"admin":"eosoramar","idevilbeha":"1","memo":"i checked, i am late", "status":"5"}' -p eosoramar
cleos -u http://127.0.0.1:8888/ --wallet-url http://127.0.0.1:8890/ get  currency balance octoneos answera OCT
cleos -u http://127.0.0.1:8888/ --wallet-url http://127.0.0.1:8890/ get  currency balance octoneos eosoramar OCT


cleos -u http://127.0.0.1:8888/ --wallet-url http://127.0.0.1:8890/  get table eosoramar answera mortgaged
cleos -u http://127.0.0.1:8888/ --wallet-url http://127.0.0.1:8890/  get table eosoramar ocaskans contractinfo
cleos -u http://127.0.0.1:8888/ --wallet-url http://127.0.0.1:8890/  get table eosoramar answera userscores
cleos -u http://127.0.0.1:8888/ --wallet-url http://127.0.0.1:8890/  get table eosoramar eosoramar behsco
