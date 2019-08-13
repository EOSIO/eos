---
title: "get accounts"
excerpt: "Retrieves all accounts associated with a defined public key."
---
[[info]]
|This command will not return privileged accounts.

## Positional Parameters
`public_key` _TEXT_  - The public key to retrieve accounts for
## Options
There are no options for this command 
## Example


```shell
$ cleos get accounts EOS8mUftJXepGzdQ2TaCduNuSPAfXJHf22uex4u41ab1EVv9EAhWt
{
  "account_names": [
    "testaccount"
  ]
}
```
