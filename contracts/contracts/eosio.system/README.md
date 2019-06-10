eosio.system
----------

This contract provides multiple functionalities:
- Users can stake tokens for CPU and Network bandwidth, and then vote for producers or delegate their vote to a proxy.
- Producers register in order to be voted for, and can claim per-block and per-vote rewards.
- Users can buy and sell RAM at a market-determined price.
- Users can bid on premium names.
- A resource exchange system (REX) allows token holders to lend their tokens, and users to rent CPU and Network resources in return for a market-determined fee. 

Actions:
The naming convention is codeaccount::actionname followed by a list of paramters.

## eosio::regproducer producer producer_key url location
   - Indicates that a particular account wishes to become a producer
   - **producer** account registering to be a producer candidate
   - **producer_key** producer account public key
   - **url** producer URL
   - **location** currently unused index

## eosio::voteproducer voter proxy producers
   - **voter** the account doing the voting
   - **proxy** proxy account to whom voter delegates vote
   - **producers** list of producers voted for. A maximum of 30 producers is allowed
   - Voter can vote for a proxy __or__ a list of at most 30 producers. Storage change is billed to `voter`.

## eosio::regproxy proxy is_proxy
   - **proxy** the account registering as voter proxy (or unregistering)
   - **is_proxy** if true, proxy is registered; if false, proxy is unregistered
   - Storage change is billed to `proxy`.
   
## eosio::delegatebw from receiver stake\_net\_quantity stake\_cpu\_quantity transfer
   - **from** account holding tokens to be staked
   - **receiver** account to whose resources staked tokens are added
   - **stake\_net\_quantity** tokens staked for NET bandwidth
   - **stake\_cpu\_quantity** tokens staked for CPU bandwidth
   - **transfer** if true, ownership of staked tokens is transfered to `receiver`
   - All producers `from` account has voted for will have their votes updated immediately.

## eosio::undelegatebw from receiver unstake\_net\_quantity unstake\_cpu\_quantity
   - **from** account whose tokens will be unstaked
   - **receiver** account to whose benefit tokens have been staked
   - **unstake\_net\_quantity** tokens to be unstaked from NET bandwidth
   - **unstake\_cpu\_quantity** tokens to be unstaked from CPU bandwidth
   - Unstaked tokens are transferred to `from` liquid balance via a deferred transaction with a delay of 3 days.
   - If called during the delay period of a previous `undelegatebw` action, pending action is canceled and timer is reset.
   - All producers `from` account has voted for will have their votes updated immediately.
   - Bandwidth and storage for the deferred transaction are billed to `from`.

## eosio::onblock header
   - This special action is triggered when a block is applied by a given producer, and cannot be generated from
     any other source. It is used increment the number of unpaid blocks by a producer and update producer schedule.

## eosio::claimrewards producer
   - **producer** producer account claiming per-block and per-vote rewards
   
## eosio::deposit owner amount
   - Deposits tokens to user REX fund
   - **owner** REX fund owner account
   - **amount** amount of tokens to be deposited
   - An inline transfer from 'owner' liquid balance is executed.
   - All REX-related costs and proceeds are deducted from and added to 'owner' REX fund, with one exception being buying REX using staked tokens.
   - Storage change is billed to 'owner'.

## eosio::withdraw owner amount
   - Withdraws tokens from user REX fund
   - **owner** REX fund owner account
   - **amount** amount of tokens to be withdrawn
   - An inline transfer to 'owner' liquid balance is executed.

## eosio::buyrex from amount
   - Buys REX in exchange for tokens taken out of user REX fund
   - **from** owner account name
   - **amount** amount of tokens to be used for purchase
   - 'amount' tokens are taken out of 'from' REX fund.
   - User must vote for at least 21 producers or delegate vote to proxy before buying REX.
   - Tokens used in purchase are added to user's voting power.
   - Bought REX cannot be sold before 4 days counting from end of day of purchase.
   - Storage change is billed to 'from' account.
   - By buying REX, user is lending tokens in order to be rented as CPU or NET resourses.

## eosio::unstaketorex owner receiver from\_net from\_cpu
   - Buys REX using staked tokens
   - **owner** owner of staked tokens
   - **receiver** account name that tokens have previously been staked to
   - **from_net** amount of tokens to be unstaked from NET bandwidth and used for REX purchase
   - **from_cpu** amount of tokens to be unstaked from CPU bandwidth and used for REX purchase
   - User must vote for at least 21 producers or delegate vote to proxy before buying REX.
   - Tokens used in purchase are added to user's voting power.
   - Bought REX cannot be sold before 4 days counting from end of day of purchase.
   - Storage change is billed to 'owner' account.

## eosio::sellrex from rex
   - Sells REX in exchange for core tokens
   - **from** owner account of REX
   - **rex** amount of REX to be sold
   - Proceeds are deducted from user's voting power.
   - If cannot be processed immediately, sell order is added to a queue and will be processed within 30 days at most.
   - In case sell order is queued, storage change is billed to 'from' account.

## eosio::cnclrexorder owner
   - Cancels unfilled REX sell order by owner if one exists.
   - **owner** owner account name

## eosio::mvtosavings owner rex
   - Moves REX to owner's REX savings bucket
   - REX held in savings bucket does not mature and cannot be sold directly
   - REX is moved out from the owner's maturity buckets as necessary starting with the bucket with furthest maturity date
   - **owner** owner account of REX
   - **rex** amount of REX to be moved to savings bucket

## eosio::mvfrsavings owner rex
   - Moves REX from owner's savings bucket to a bucket with a maturity date that is 4 days after the end of the day
   - This action is required if the owner wants to sell REX held in savings bucket
   - **owner** owner account of REX
   - **rex** amount of REX to be moved from savings bucket

## eosio::rentcpu from receiver loan\_payment loan\_fund
   - Rents CPU resources for 30 days in exchange for market-determined price
   - **from** account creating and paying for CPU loan
   - **receiver** account receiving rented CPU resources
   - **loan_payment** tokens paid for the loan
   - **loan_fund** additional tokens (can be zero) added to loan fund and used later for loan renewal
   - Rents as many core tokens as determined by market price and stakes them for CPU bandwidth for the benefit of `receiver` account.
   - `loan_payment` is used for renting, it has to be greater than zero. Amount of rented resources is calculated from `loan_payment`.
   - After 30 days the rented core delegation of CPU will expire or be renewed at new market price depending on available loan fund.
   - `loan_fund` can be zero, and is added to loan balance. Loan balance represents a reserve that is used at expiration for automatic loan renewal.
   - 'from' account can add tokens to loan balance using action `fundcpuloan` and withdraw from loan balance using `defcpuloan`.
   - At expiration, if balance is greater than or equal to `loan_payment`, `loan_payment` is taken out of loan balance and used to renew the loan. Otherwise, the loan is closed and user is refunded any remaining balance.
   
## eosio::rentnet from receiver loan\_payment loan\_fund
   - Rents Network resources for 30 days in exchange for market-determined price
   - **from** account creating and paying for Network loan
   - **receiver** account receiving rented Network resources
   - **loan_payment** tokens paid for the loan
   - **loan_fund** additional tokens (can be zero) added to loan fund and used later for loan renewal
   - Rents as many core tokens as determined by market price and stakes them for Network bandwidth for the benefit of `receiver` account.
   - `loan_payment` is used for renting, it has to be greater than zero. Amount of rented resources is calculated from `loan_payment`.
   - After 30 days the rented core delegation of Network will expire or be renewed at new market price depending on available loan fund.
   - `loan_fund` can be zero, and is added to loan balance. Loan balance represents a reserve that is used at expiration for automatic loan renewal.
   - 'from' account can add tokens to loan balance using action `fundnetloan` and withdraw from loan balance using `defnetloan`.
   - At expiration, if balance is greater than or equal to `loan_payment`, `loan_payment` is taken out of loan balance and used to renew the loan. Otherwise, the loan is closed and user is refunded any remaining balance.

## eosio::fundcpuloan from loan\_num payment
   - Transfers tokens from REX fund to the fund of a specific CPU loan in order to be used for loan renewal at expiry
   - **from** loan creator account
   - **loan_num** loan id
   - **payment** tokens transfered from REX fund to loan fund

## eosio::fundnetloan from loan\_num payment
   - Transfers tokens from REX fund to the fund of a specific Network loan in order to be used for loan renewal at expiry
   - **from** loan creator account
   - **loan_num** loan id
   - **payment** tokens transfered from REX fund to loan fund

## eosio::defcpuloan from loan\_num amount
   - Withdraws tokens from the fund of a specific CPU loan and adds them to REX fund
   - **from** loan creator account
   - **loan_num** loan id
   - **amount** tokens transfered from CPU loan fund to REX fund

## eosio::defcpuloan from loan\_num amount
   - Withdraws tokens from the fund of a specific CPU loan and adds them to REX fund
   - **from** loan creator account
   - **loan_num** loan id
   - **amount** tokens transfered from NET loan fund to REX fund

## eosio::updaterex owner
   - Updates REX owner vote weight to current value of held REX
   - **owner** REX owner account

## eosio::rexexec user max
   - Performs REX maintenance by processing a specified number of REX sell orders and expired loans
   - **user** any account can execute this action
   - **max** number of each of CPU loans, NET loans, and sell orders to be processed

## eosio::consolidate owner
   - Consolidates REX maturity buckets into one bucket that cannot be sold before 4 days
   - **owner** REX owner account name

## eosio::closerex owner
   - Deletes unused REX-related database entries and frees occupied RAM
   - **owner** user account name
   - If owner has a non-zero REX balance, the action fails; otherwise, owner REX balance entry is deleted.
   - If owner has no outstanding loans and a zero REX fund balance, REX fund entry is deleted.
