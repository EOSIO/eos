---
content_title: EOSIO v2.0.12 Release Notes
---

This release contains security updates and miscellaneous fixes.

## Security updates

### Consolidated Security Updates for v2.0.12 ([#10264](https://github.com/EOSIO/eos/pull/10264))
- Apply three-strikes rule to all transaction failures
- Apply unconditional subjective CPU check along with some additional logging
- Provide options to enable subjective CPU billing for P2P and API transactions ,and provide an option to disable it for individual accounts

This release expands upon the subjective CPU billing introduced in ([v2.0.10](https://github.com/EOSIO/eos/tree/v2.0.10)). Subjective billing (disabled by default) can now be applied to transactions that come in from either P2P connections, API requests, or both. By setting `disable-subjective-billing` to `false` both P2P and API transactions will have subjective CPU billing applied. Using `disable-subjective-p2p-billing` and/or `disable-subjective-api-billing` will allow subjective CPU billing to be enabled/disabled for P2P transactions or API transactions respectively. Another option , `disable-subjective-account-billing = <account>`, is used to selectively disable subjective CPU billing for certain accounts while applying subjective CPU billing to all other accounts.

`cleos get account` is enhanced to report `subjective cpu bandwidth`, which contains used subjective CPU billing in microseconds for a particular account on a given node.

Note: These security updates are relevant to all nodes on EOSIO blockchain networks.


## Other changes
- ([#10155](https://github.com/EOSIO/eos/pull/10155)) [2.0.x] Improve timeouts occurring on Anka builds.
- ([#10171](https://github.com/EOSIO/eos/pull/10171)) Wlb/ctest generalization for parameter tests 2.0.x
- ([#10233](https://github.com/EOSIO/eos/pull/10233)) Support Running Version Tests on Fresh OS Installs
- ([#10244](https://github.com/EOSIO/eos/pull/10244))  migrate boost downloads from BinTray to JFrog Artifactory - 2.0
- ([#10250](https://github.com/EOSIO/eos/pull/10250)) Rel 2.0.x: Subjective CPU billing cleos enhancement and adding subjective_cpu_bill to /v1/chain/get_account result
## Documentation
- ([#10186](https://github.com/EOSIO/eos/pull/10186)) Add EOSIO 2.0.11 release notes to dev portal - 2.0
