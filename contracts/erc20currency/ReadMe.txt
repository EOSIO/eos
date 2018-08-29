step1.Create account
1.1 Create keys
doushadeMacBook-Pro:doc cl$ cleos create key
Private key: 5KYRmJzV2eRswvdQ6rvkeDMBWyh8Vq33FUY8PVmAZUbvAVeukzU
Public key: EOS8gb6EkQDry72Ugwwy6MUZCr7EQ4ytGAeRhu3knVsA3qSneWPEb
doushadeMacBook-Pro:doc cl$ cleos create key
Private key: 5Jqs5XFBqC787ywHjfUiT66HrEug5RCcqwgBsFYAX6nz8GU6W8t
Public key: EOS6BB3rfssiBdiBN2d14GQLswcM1KBmcKWEM8fiSmdLw6eKWP5et

1.2 Import eosio private key,and then create three accouts, who will be used
cleos wallet import 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3

cleos create account eosio erctzcur EOS8gb6EkQDry72Ugwwy6MUZCr7EQ4ytGAeRhu3knVsA3qSneWPEb EOS6BB3rfssiBdiBN2d14GQLswcM1KBmcKWEM8fiSmdLw6eKWP5et
cleos create account eosio chenlian EOS8gb6EkQDry72Ugwwy6MUZCr7EQ4ytGAeRhu3knVsA3qSneWPEb EOS6BB3rfssiBdiBN2d14GQLswcM1KBmcKWEM8fiSmdLw6eKWP5et
cleos create account eosio xiaoli EOS8gb6EkQDry72Ugwwy6MUZCr7EQ4ytGAeRhu3knVsA3qSneWPEb EOS6BB3rfssiBdiBN2d14GQLswcM1KBmcKWEM8fiSmdLw6eKWP5et
cleos create account eosio octadmin EOS8gb6EkQDry72Ugwwy6MUZCr7EQ4ytGAeRhu3knVsA3qSneWPEb EOS6BB3rfssiBdiBN2d14GQLswcM1KBmcKWEM8fiSmdLw6eKWP5et

step2.Compile contract erc20currency.cpp to erctzcur,then publish contract
2.1 Compile contract
eosiocpp -o erc20currency.wast erc20currency.cpp

2.2 Import erctzcur chenlian xiaoli private key
cleos wallet import 5Jqs5XFBqC787ywHjfUiT66HrEug5RCcqwgBsFYAX6nz8GU6W8t

2.3 Publish contract
cleos set contract erctzcur ./ erc20currency.wast erc20currency.abi -p erctzcur

step3.Create one kind of currency with name OCT,then test it
3.1 create currency OCT
cleos push action erctzcur create '[ "octadmin", "1000000000.0000 OCT", 0, 0, 0]' -p octadmin

3.2 Issue 500 OCT to chenlian
cleos push action erctzcur issue '[ "chenlian", "500.0000 OCT", "m" ]' -p octadmin

3.3 Try to get chenlian balance(just test 3.2)
cleos get table erctzcur chenlian accounts

3.4 Test transfer action
cleos push action erctzcur transfer '[ "chenlian", "xiaoli", "2.0000 OCT", "m" ]' -p chenlian

3.5 Try to get xiaoli balance(just test 3.4)
cleos get table erctzcur xiaoli accounts

3.6 Test approve action
cleos push action erctzcur approve '{"owner":"chenlian", "spender":"xiaoli", "quantity":"2.0000 OCT"}' -p chenlian

3.7 Try to get chenlian->xiaoli allowance(just test 3.6)
cleos get table erctzcur chenlian approves

3.8 Test transferFrom
cleos push action erctzcur transferfrom '{"from":"chenlian", "to":"xiaoli", "quantity":"2.0000 OCT"}' -p xiaoli

3.9 Get approve about account chenlian(just check 3.8)
cleos get table erctzcur chenlian approves
cleos get table erctzcur chenlian accounts
cleos get table erctzcur xiaoli accounts

3.10 Get balance account by balanceof action,compare with the result by get table
cleos push action erctzcur balanceof '{"owner":"chenlian", "symbol":"OCT"}' -p chenlian
cleos get table erctzcur chenlian accounts

3.11 Get allowance about account by allowanceof action,compare with the result by get table
cleos push action erctzcur allowanceof '{"owner":"chenlian", "spender":"xiaoli", "symbol":"OCT"}' -p chenlian
cleos get table erctzcur chenlian approves
