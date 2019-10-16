from datetime import datetime, timezone

from mainnet_bootstrap import system_accounts, remcli, run, jsonArg, intToRemCurrency
from eosiopy import sign

producers_quantity = 21
rewards_account_supply = 100_000_000_0000
producers_supply = 500_000_000_0000
swapbot_supply = 1_000_000_0000
fee = 21*100_0000+100_0000

MINIMUM_ACCOUNT_STAKE = 500_0000
MINIMUM_PRODUCER_STAKE = 250_000_0000

tech_accounts = {
    'rewards': 'EOS8Znrtgwt8TfpmbVpTKvA2oB8Nqey625CLN8bCN3TEbgx86Dsvr',
}

producers = {
    "producers": [
        {"name": "remproducer1", "pvt": "5KLGj1HGRWbk5xNmoKfrcrQHXvcVJBPdAckoiJgFftXSJjLPp7b",
         "pub": "EOS8imf2TDq6FKtLZ8mvXPWcd6EF2rQwo8zKdLNzsbU9EiMSt9Lwz"},
        {"name": "remproducer2", "pvt": "5K6qk1KaCYYWX86UhAfUsbMwhGPUqrqHrZEQDjs9ekP5j6LgHUu",
         "pub": "EOS7Ef4kuyTbXbtSPP5Bgethvo6pbitpuEz2RMWhXb8LXxEgcR7MC"},
        {"name": "remproducer3", "pvt": "5JCStvbRgUZ6hjyfUiUaxt5iU3HP6zC1kwx3W7SweaEGvs4EPfQ",
         "pub": "EOS5n442Qz4yVc4LbdPCDnxNSseAiUCrNjRxAfPhUvM8tWS5svid6"},
        {"name": "remproducer4", "pvt": "5JJjaKnAb9KM2vkkJDgrYXoeUEdGgWtB5WK1a38wrmKnS3KtkS6",
         "pub": "EOS5y3Tm1czTCia3o3JidVKmC78J9gRQU8qHjmRjFxTyhh2vxvF5d"},
        {"name": "remproducer5", "pvt": "5K7hmHA2U3nNpwGx6AffWsHyvsSMJvVKVmSgxnSYAjAvjUfzd5j",
         "pub": "EOS5yR5GNn363W3cnet5PE6xWZxa2hDAhmJN5RrB1e3fmJmVNnDRJ"},
        {"name": "remproduce11", "pvt": "5K3TXkZAwyJkg7TjSfopd7sTr3RXFccghXHN1nJHTzap1ZKgLdK",
         "pub": "EOS76wwc1zjzRMPQnEL6rTDWLfhN2ByZd6GhDJoWBKWM6M7cUmyfS"},
        {"name": "remproduce12", "pvt": "5J498VQKaFVbh2ovPrMxa5DThE1fiWWfna6H2ZofmGMaVM7bs2M",
         "pub": "EOS63piLomgbAGShNu7zv3whiR52Abz8gVVMRNAqw4apJ8EQKEnQ5"},
        {"name": "remproduce13", "pvt": "5JrC4vnVXroqvXtmSw3EiXcSv9agfDsKsUghzv2Rhiyrg9J36Z4",
         "pub": "EOS8ZjbDEi872aLpuuAjnd76NYW6KzPaf6RBSuwXcHmKm7A1sxayV"},
        {"name": "remproduce14", "pvt": "5Jx1cCRPwHM7mr8brbeS6k5GYFEDKgmz34kmfmhywToF5wqddQE",
         "pub": "EOS7oiy488wfzwbR6L9QwNDQb4CHGVR5ix9udJjVyykgbLaQLgBc5"},
        {"name": "remproduce15", "pvt": "5Hw5iBC7oGfR3FTwgDzHuzk4JAv24SiXq9iBbyPMb8JmKBHzjVX",
         "pub": "EOS6iBVEaRDS3mofUJpa8bjD94vohHrsSqqez1wcsWjhPpeNArfBF"},
        {"name": "remproduce21", "pvt": "5J3Zd6iSGcFFzRkUUqG7nKV8xifZb3sZvCnnst2WYgKqsRMvHTF",
         "pub": "EOS8RpmTrNN7eEFno626tNutAKJbJ3TbMMyHi9sbmpU9t4vEYDAYM"},
        {"name": "remproduce22", "pvt": "5JxWkyTDwktJVs9MNgNgmaLrRc26ESswR9gk926g47t6UCqvmop",
         "pub": "EOS8jxiWA9ZHN37csWnoShEbs8tvgtYxt2UN4ZtMBXhGJRk8tQJqR"},
        {"name": "remproduce23", "pvt": "5HwyQG6enrCrCZXYqrd2WsMmnreWAWUWGa9JjssodsYTSuaSA5F",
         "pub": "EOS6u4zPBn1EbhNtqmSFgUhoo1ZHb1je2qUskRgk4jUe2nUfpFXLP"},
        {"name": "remproduce24", "pvt": "5JyvFNY4xMHX96UFjA1sNbLTRSom5zuLSn83KYXSVu1Ex4vuY9m",
         "pub": "EOS4uUK6JSXyBHNCGYEzrZzepkMSHzJsaXBAHFsieGGQ1Syzpxy11"},
        {"name": "remproduce25", "pvt": "5KNe71b8F6zUszs2Mg2GPZa5cUse5i9Uy9EeH1g692aCq7KnGk9",
         "pub": "EOS7iKtJ1qCgSaGfC8fquHquQYZmi43RQTkNWn68ggCTD5gUZjmYw"},
        {"name": "remproduce31", "pvt": "5K61WJJWJ2ua4nz6EmgUMyAgEnRVc1MdvrHzi4PdVaPHV7R3r4M",
         "pub": "EOS7KsEiRyppoaxv5yBW666b3MpDRq7FpQBttU2UUher5Ur2ke3pB"},
        {"name": "remproduce32", "pvt": "5KAiF4UX7kLHeScvA7fqrSdfxKycEZ2y76uRCRD78kFpdUeq2xm",
         "pub": "EOS5kiGwBZQgvY8WibiAprDbXZF1KmgEgWzwfQbAP6LFDLm5PHkyH"},
        {"name": "remproduce33", "pvt": "5JwfitftPcR1eMfNuo4ZqeKYECkFCZBbPYC9zH51MK7i87packn",
         "pub": "EOS66ncp9xAzdCaAggUDChFMq9YX9wfAJQmN5uMdxf8APV4ecqaZV"},
        {"name": "remproduce34", "pvt": "5JyVEkLeSgioeMcw7sBLDHam4xxxDWMgaVnaBEmdWbHr9Xuo3oT",
         "pub": "EOS8VPgtZmjHmPXrSmY2Mita3zevnaKDHPiiyJHTQPbf4cMdA3EiK"},
        {"name": "remproduce35", "pvt": "5Jj5MMoPGp4H5bC5L6XAvDFhGGok2ghTd4mJxs9toihBYJf3oKX",
         "pub": "EOS6wLPhQzGE96dMNXZxSN9GFnNtHNXY5q6CZ1BY4CizWScLvHCH6"},
        {"name": "remproduce41", "pvt": "5KGWWthBagnELkrn71W4q1CtaExxasmM9X2eV8AUvjMCb1kgLHG",
         "pub": "EOS7M3GEpdYZhWhKaehthCfbyQBZfHwxDbZ9ewEGysscchkHv2WCq"},
        {"name": "remproduce42", "pvt": "5JEEVueiBZDXVQCTRduZnTs6hLUA29dG76p8NEQqWSZ2Fg4TkbD",
         "pub": "EOS83fUneyFirnM82JT6PxgbJp5ZMxj757YJDQLFPQHKvaezRNDoX"},
        {"name": "remproduce43", "pvt": "5KNMTQ3qZMH9LBjBqRMmDTi8DTGbQYYbJv1p5a58nGcYqDzaRJ5",
         "pub": "EOS7rGbRupx3EkQo7dTcE9Ryx6XnEeNSEHwFABveDGtN7wUK4Xn9Q"},
        {"name": "remproduce44", "pvt": "5K5KAPehBnqQD3sRryjXWfTYaPDfmMfAAYauNk49tSf2gXfCEQo",
         "pub": "EOS7Xk7EhYdvWds1qkfdiTaaXYJyupJRm9Ufj5dba6QiYPqQT57hU"},
        {"name": "remproduce45", "pvt": "5Jo3yZyJJwaoGzSRVi6PimiuxA3HE48Tyti7X1BmQrtt3dx2szH",
         "pub": "EOS8erdpbAGK1Xbuz2zdxTY7Km2wHZfL6znXn1ijFJLPn1AR2ZzZx"},
        {"name": "remproduce51", "pvt": "5JsP33C6e9edyFC6QWp361Q9ZVNKWKSznQ6198PkP1c8FpjQcdx",
         "pub": "EOS64UfKJXespdvq4uYtN4rKEuSMuqRXq6rXaNkH8VCXP7iE1Bjdg"},
        {"name": "remproduce52", "pvt": "5JDuKWcJFiwtogspSusdqcHWsHKEn5HydApKL58GaVNdnvQh1m1",
         "pub": "EOS7DUtP3y2yqeabTYavbuFvA5yfSLM1fg7t3JVRr4dMuEJ4GZWZS"},
        {"name": "remproduce53", "pvt": "5KD3hRFXkMYYUurVC3myJo9FRmT9d9XidyNXYSVTKtQ4wegh4Uw",
         "pub": "EOS6K1vpE6XAX39kKAu2tBD6jJ8XpQzjWhC3X92duyQNBvdrk2zmr"},
        {"name": "remproduce54", "pvt": "5JYamJxxJiB8aJhn4W5G5XUxzBd8HRNWoQop7SLNhgnf6LAiquF",
         "pub": "EOS7ogfqTFzUe97Rr6taa3SmqrqQJujMmjMHgZ72TfPZFXqVWPjSa"},
        {"name": "remproduce55", "pvt": "5KZFvhuNuU3es7hEoAorppkhfCuAfqBGGtzqvesArmzwVwJf64B",
         "pub": "EOS69tWc1VS6aP2P1D8ryzTiakPAYbV3whbHeWUzfD8QWYuHKqQxk"}
    ]
}
prod_supply = producers_supply//2
prod_supply_sum = 0
for prod in producers['producers']:
    prod['funds'] = max(prod_supply, MINIMUM_PRODUCER_STAKE)
    prod_supply_sum += prod['funds']
    prod_supply = prod_supply // 2

initial_supply = rewards_account_supply + prod_supply_sum + swapbot_supply + fee

swap_pubkey = 'EOS8Znrtgwt8TfpmbVpTKvA2oB8Nqey625CLN8bCN3TEbgx86Dsvr'
swap_privkey = '5K463ynhZoCDDa4RDcr63cUwWLTnKqmdcoTKTHBjqoKfv4u5V7p'


def create_tech_accounts():
    for account_name, public_key in tech_accounts.items():
        run(remcli + f' system newaccount rem {account_name} {public_key} {public_key} \
        --stake "{intToRemCurrency(MINIMUM_ACCOUNT_STAKE)}" --transfer -p rem@active')


def init_supply_to_rem_acc():
    timestamp = datetime.today().strftime("%Y-%m-%dT%H:%M:%S")
    epoch = datetime.utcfromtimestamp(0)
    block_datetime_utc = datetime.strptime(timestamp, '%Y-%m-%dT%H:%M:%S').replace(tzinfo=timezone.utc).timestamp()
    epoch_datetime_utc = datetime.utcfromtimestamp(0).replace(tzinfo=timezone.utc).timestamp()
    seconds_since_epoch = str(int(block_datetime_utc - epoch_datetime_utc))

    txid = '0x0000000000000000000000000000000000000000000000000000000000000000'
    return_address = '0x0000000000000000000000000000000000000000'
    chain_id = '93ece941df27a5787a405383a66a7c26d04e80182adf504365710331ac0625a7'
    return_chainid = 'eth'
    rampayer = 'rem'
    receiver = 'rem'

    digest_to_sign = f'{receiver}*{txid}*{chain_id}*{intToRemCurrency(initial_supply)}*\
{return_address}*{return_chainid}*{seconds_since_epoch}'.encode()

    sig = sign(swap_privkey, digest_to_sign)

    run(remcli + f' push action rem.swap init \'["{rampayer}",\
    "{txid}", "{swap_pubkey}",\
    "{intToRemCurrency(initial_supply)}", "{return_address}", "{return_chainid}", "{timestamp}"]\'\
    -p rem@active')

    run(remcli + f' push action rem.swap finish \'["{rampayer}", "{receiver}",\
    "{txid}",\
    "{swap_pubkey}", "{intToRemCurrency(initial_supply)}", "{return_address}", "{return_chainid}",\
    "{timestamp}", "{sig}"]\'\
    -p rem@active')


def create_producer_accounts():
    for prod in producers['producers']:
        run(remcli + f' system newaccount rem {prod["name"]} {prod["pub"]} {prod["pub"]} \
        --stake "{intToRemCurrency(MINIMUM_ACCOUNT_STAKE)}" --transfer -p rem@active')


def import_producer_keys():
    for prod in producers['producers']:
        run(remcli + 'wallet import --private-key ' + prod['pvt'])


def transfer_tokens_to_accounts():
    for prod in producers['producers']:
        run(remcli + f' transfer rem {prod["name"]} "{intToRemCurrency(prod["funds"] - MINIMUM_ACCOUNT_STAKE)}" ""')

    run(remcli + f' transfer rem swapbot "{intToRemCurrency(swapbot_supply)}" ""')
    run(remcli + f' transfer rem rewards "{intToRemCurrency(rewards_account_supply)}" ""')


def stake_tokens():
    for prod in producers['producers']:
        run(remcli + f' system delegatebw {prod["name"]} {prod["name"]} \
        "{intToRemCurrency(prod["funds"] - MINIMUM_ACCOUNT_STAKE)}" -p {prod["name"]}@active')

    run(remcli + f' system delegatebw swapbot swapbot \
    "{intToRemCurrency(swapbot_supply)}" -p swapbot@active')

    run(remcli + f' system delegatebw rewards rewards \
    "{intToRemCurrency(rewards_account_supply)}" -p rewards@active')


def reg_producers():
    for prod in producers['producers']:
        run(remcli + f' system regproducer {prod["name"]} {prod["pub"]} "" \
        -p {prod["name"]}@active')


def vote_producers():
    for prod in producers['producers']:
        run(remcli + f' system voteproducer prods {prod["name"]} {prod["name"]} \
        -p {prod["name"]}@active')


if __name__ == '__main__':
    init_supply_to_rem_acc()
    create_tech_accounts()
    create_producer_accounts()
    import_producer_keys()
    transfer_tokens_to_accounts()
    stake_tokens()
    reg_producers()
    vote_producers()
