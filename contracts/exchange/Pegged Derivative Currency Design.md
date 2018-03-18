# Pegged Derivative Currency Design

A currency is designed to be a fungible and non-callable asset. A pegged Derivative currency, such as BitUSD, is backed by a cryptocurrency held as collateral. The "issuer" is "short" the dollar and extra-long the cryptocurrency. The buyer is simply long the dollar.  



Background
----------
BitShares created the first working pegged asset system by allowing anyone to take out a short position by posting collateral and issuing BitUSD at a minimum 1.5:1 collateral:debt ratio. The **least collateralized position** was forced to provide liquidity for BitUSD holders
any time the market price fell more than a couple percent below the dollar (if the BitUSD holder opted to use forced liquidation).

To prevent abuse of the price feed, all forced liquidation was delayed.

In the event of a "black swan" all shorts have their positions liquidated at the price feed and all holders of BitUSD are only promised a fixed redemption rate.

There are several problems with this design:

1. There is very **poor liquidity** in the BitUSD / BitShares market creating **large spreads**
2. The shorts take all the risk and only profit when the price of BitShares rises
3. Blackswans are perpetual and very disruptive.
4. It is "every short for themselves" 
5. Due to the risk/reward ratio the supply can be limited
6. The **collateral requirements** limit opportunity for leverage.

New Approach
------------
We present a new approach to pegged assets where the short-positions cooperate to provide the
service of a pegged asset with **high liquidity**. They make money by encouraging people to trade
their pegged asset and earning income **from the trading fees rather than seeking heavy leverage**
in a speculative market. They also generate money by earning interest on personal short positions.

The Setup Process
-----------------
An initial user deposits a Collateral Currency (C) into an smart contract and provides the initial
price feed. A new Debt token (D) is issued based upon the price feed and a 1.5:1 C:D ratio and the
issued tokens are deposited into the **Bancor market maker**. At this point in time there is 0 leverage by
the market maker because no D have been sold. The initial user is also issued exchange tokens (E) in the
market maker.

At this point people can buy E or D and the Bancor algorithm will provide liquidity between C, E, and D. Due to
the fees charged by the the market maker the value of E will increase in terms of C.

> Collateral currency = Smart Token/reserve of parent currency
>
> Issued tokens = Bounty Tokens (distributed to early holders / community supporters)
>
> Collateral Ratio (C:D) = reciprocal of Loan-to-Value Ratio (LTV) 

Maintaining the Peg
-------------------
To maximize the utility of the D token, the market maker needs to maintain a **narrow trading range** of D vs the Dollar. 
The more **consistant and reliable this trading range** is, the more people (arbitrageur) will be willing to hold and trade D. There are several
situations that can occur:

1. D is trading above a dollar +5% 

   a. Maker is fully collateralized `C:D>1.5`

       - issue new D and deposit into maker such that collateral ratio is 1.5:1
   b. Maker is not fully collateralized `C:D<1.5`
   
       - adjust the maker weights to lower the redemption prices (defending capital of maker), arbitrageur will probably prevent this reality.

   > Marker Weights = Connector Weights (in Bancor)
   >
   > Redemption Price: The price at which a bond may be repurchased by the issuer before maturity

2. D is selling for less than a dollar -5%

   a. Maker is fully collateralized `C:D>1.5`

       - adjust the maker weights to increase redemption prices 
   b. Maker is under collateralized `C:D<1.5`
   ```
       - stop E -> C and E -> D trades.
       - offer bonus on C->E and D->E trades.
       - on D->E conversions take received D out of circulation rather than add to the market maker
       - on C<->D conversion continue as normal
       - stop attempting adjusting maker ratio to defend the price feed and let the price rise until above +1%
   ```

Value of E = C - D  where D == all in circulation, so E->C conversions should always assume all outstanding D was **settled at current maker price**. The result of such a conversion will **raise the collateral ratio**, unless they are forced to buy and retire some D at the current ratio. The algorithm must ensure the individual selling E doesn't leave those holding E worse-off from a D/E perspective (doesnot reduce D to a large extent).  An individual buying E will create new D to maintain the same D/E ratio.

This implies that when value of all outstanding D is greater than all C that E cannot be sold until the network
generates **enough in trading fees** to recaptialize the market. This is like a company with more debt than equity not allowing buybacks. In fact, **E should not be sellable any time the collateral ratio falls below 1.5:1**. 

BitShares is typical **margin call** territory, but holders of E have a chance at future liquidity if the situation improves. While E is not sellable,
E can be purchased at a 10% discount to its theoretical value, this will dilute existing holders of E but will raise capital and hopefully move E holders closer to eventual liquidity. 


Adjusting Bancor Ratios by Price Feed
-------------------------------------
The price feed informs the algorithm of significant deviations between the Bancor effective price and the target peg. The price feed is necessarily a lagging indicator and may also factor in natural spreads between different exchanges. Therefore, the price feed shall have no impact unless there is a significant deviation (5%). When such a deviation occurs, the ratio is automatically adjusted to 4%.
    
In other words, the price feed keeps the maker in the "channel" but does not attempt to set the real-time prices. If there is a sudden change and the price feed differs from maker by 50% then after the adjustment it will still differ by 4%.  

> Effective Price = Connected Tokens exchanged / Smart Tokens exchanged

Summary
-------
Under this model holders of E are short the dollar and make money to recollateralize their positions via market activity. 
Anyone selling E must **realize the losses as a result of being short**.    
Anyone buying E can get in to take their place at the current collateral ratio.

The value of E is equal to the value of a **margin postion**.     
Anyone can buy E for a combination C and D equal to the current collateral ratio.

Anyone may sell E for a personal margin position with equal ratio of C and D.    
Anyone may buy E with a personal margin position. 

If they only have C, then they must use some of C to buy D first (which will move the price).    
If they only have D, then they must use some of D to buy C first (which will also move the price).

Anyone can buy and sell E based upon Bancor balances of C and (all D), they must sell their E for a combination of D and C at current ratio, then sell the C or D for the other. 


Anytime collateral level falls below 1.5 selling E is blocked and buying of E is given a 10% bonus.   
Anyone can convert D<->C using Bancor maker configured to maintain price within +/- 5% of the price feed.                                                                    


