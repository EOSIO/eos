how_to = [
'How-to-submit-a-transaction',
'How-to-transfer-an-eosio.token',
'How-to-create-an-account',
'How-to-stake-resource',
'How-to-unstake',
'How-to-link-permission',
'How-to-unlink-permission',
'How-to-get-block-information',
'How-to-get-account-information',
'How-to-get-transaction-information',
'How-to-get-tables-information',
'How-to-deploy-a-smart-contract',
'How-to-create-key-pairs',
'How-to-create-a-wallet',
'How-to-import-a-key',
'How-to-sign-a-multisig-transaction',
'How-to-vote',
'How-to-set-the-network'
]

how_to.each {|name| File.open "#{name.downcase}.MD", "w"} 