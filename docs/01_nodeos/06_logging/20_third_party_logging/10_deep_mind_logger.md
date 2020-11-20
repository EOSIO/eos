---
content_title: Deep-mind Logger Integration
link_text: Deep-mind Logger Integration
---

## Overview

The `Deep-mind logger` is part of the `dfuse` [platform]([https://dfuse.io/](https://dfuse.io/)) which is a highly scalable and performant [open-source]([https://github.com/dfuse-io/dfuse-eosio/tree/master](https://github.com/dfuse-io/dfuse-eosio/tree/master)) platform for searching and processing blockchain data.

### How To Enable Deep-mind Logger

EOSIO integrates the `nodeos` core service daemon with `deep-mind logger`. To benefit from full `deep-mind` logging functionality you must start your `nodeos` instance with the flag `--deep-mind`. After the start you can observe in the `nodeos` console output the informative details outputs created by the `deep-mind` logger. They distinguish themselves from the default `nodeos` output lines because they start with the `DMLOG` keyword.

Examples of `deep-mind` log lines as you would see them in the `nodeos` output console:

```console
DMLOG START_BLOCK 30515

DMLOG TRX_OP CREATE onblock 308f77bf49ab4ddde74d37c7310c0742e253319d9da57ebe51eb7b35f1ffe174 {"expiration":"2020-11-12T10:13:06","ref_block_num":30514,...}

DMLOG CREATION_OP ROOT 0

DMLOG RLIMIT_OP ACCOUNT_USAGE UPD {"owner":"eosio","net_usage":{"last_ordinal":1316982371,"value_ex":0,"consumed":0},"cpu_usage":{"last_ordinal":1316982371,"value_ex":24855,"consumed":101},"ram_usage":27083}

DMLOG APPLIED_TRANSACTION 30515 {"id":"308f77bf49ab4ddde74d37c7310c0742e253319d9da57ebe51eb7b35f1ffe174","block_num":30515,"block_time":"2020-11-12T10:13:05.500",...}

DMLOG RLIMIT_OP STATE UPD {"average_block_net_usage":{"last_ordinal":30514,"value_ex":0,"consumed":0},"average_block_cpu_usage":{"last_ordinal":30514,...}
DMLOG ACCEPTED_BLOCK 30516 {"block_num":30516,"dpos_proposed_irreversible_blocknum":30516,"dpos_irreversible_blocknum":30515,...

...

DMLOG FEATURE_OP ACTIVATE 0ec7e080177b2c02b278d5088611686b49d739925a92d9bfcacd7fc6b74053bd {"feature_digest":"0ec7e080177b2c02b278d5088611686b49d739925a92d9bfcacd7fc6b74053bd","subjective_restrictions":{"enabled":true,"preactivation_required":false,"earliest_allowed_activation_time":"1970-01-01T00:00:00.000"},"description_digest":"64fe7df32e9b86be2b296b3f81dfd527f84e82b98e363bc97e40bc7a83733310","dependencies":[],"protocol_feature_type":"builtin","specification":
[{"name":"builtin_feature_codename","value":"PREACTIVATE_FEATURE"}]}

...

DMLOG FEATURE_OP ACTIVATE 825ee6288fb1373eab1b5187ec2f04f6eacb39cb3a97f356a07c91622dd61d16 {"feature_digest":"825ee6288fb1373eab1b5187ec2f04f6eacb39cb3a97f356a07c91622dd61d16","subjective_restrictions":{"enabled":true,"preactivation_required":true,"earliest_allowed_activation_time":"1970-01-01T00:00:00.000"},"description_digest":"14cfb3252a5fa3ae4c764929e0bbc467528990c9cc46aefcc7f16367f28b6278","dependencies":[],"protocol_feature_type":"builtin","specification":
[{"name":"builtin_feature_codename","value":"KV_DATABASE"}]}

...

DMLOG FEATURE_OP ACTIVATE c3a6138c5061cf291310887c0b5c71fcaffeab90d5deb50d3b9e687cead45071 {"feature_digest":"c3a6138c5061cf291310887c0b5c71fcaffeab90d5deb50d3b9e687cead45071","subjective_restrictions":{"enabled":true,"preactivation_required":true,"earliest_allowed_activation_time":"1970-01-01T00:00:00.000"},"description_digest":"69b064c5178e2738e144ed6caa9349a3995370d78db29e494b3126ebd9111966","dependencies":[],"protocol_feature_type":"builtin","specification":
[{"name":"builtin_feature_codename","value":"ACTION_RETURN_VALUE"}]}
```
