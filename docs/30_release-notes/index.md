---
content_title: EOSIO v2.2.0-rc1 Release Notes
---

This is a RELEASE CANDIDATE for version 2.2.0.

The EOSIO 2.2.0-rc1 includes three new features: Read-only Queries, Private Chain Access, and Resource Payer. 

## Changes

### Read-only Queries ([#10189](https://github.com/EOSIO/eos/pull/10189))
This feature provides a contract-supported method of read-only queries from external clients via a new HTTP-RPC API endpoint `push_ro_transaction`.  Prior to this release, access was limited to data stored in a single table in DB or KV tables via the `get_tables` API, a mechanism which is exacting and inefficient. Read-only queries enable developers to perform more complex, cross-table queries natively via contract code with the assurance that table data is from the same block height. It can also be used to understand the consequences of a transaction before it is sent. For more details 
see [Read-only Queries Documentation](https://developers.eos.io/manuals/eos/v2.2/cleos/how-to-guides/how-to-call-smart-contract-read-only-queries).

### Private Chain Access Control ([#10267](https://github.com/EOSIO/eos/pull/10267))
Data access control is a critical factor for businesses and organizations interested in using blockchain technologies. In response to this need, a working group was formed between block.one and several technology providers within the community to better identify specific needs and possible solutions. This feature is the first step toward addressing the use cases identified by this group, and allows blockchain administrators to form privacy groups in order to ensure that only authorized parties can join the network or access data. This feature requires that the `SECURITY_GROUP` protocol feature is activated.

Blockchain administrators are now able to add or revoke connection access simply by modifying a privileged smart contract. Additionally, all p2p connections within privacy group are using TLS to further ensure protection of sensitive data.
For more details read [Private Chain Access documentation](https://developers.eos.io/manuals/eos/v2.2/nodeos/features/private_chain_access).

### Resource Payer ([#10407](https://github.com/EOSIO/eos/pull/10407))
Currently the resource costs for transactions on EOSIO-based public networks are paid by the end user of the application, which can make attracting and onboarding new users very difficult. There is a workaround available, but it’s not a straightforward process for either app developers or their end users. If configured per our reference, the additional transaction extension allows a third party to pay for a transactions net and cpu usage. The transaction must be signed by the payer. There is also the ability to limit the amount of cpu and net that the payer is willing to pay. The `transaction_trace` has been modified with the addition of a `bill_to_accounts` field, so that it is clear which account paid for cpu/net for the transaction. This feature requires that the `RESOURCE_PAYER` protocol feature is activated. For more details see [Resource Payer Documentation](https://developers.eos.io/manuals/eosio.cdt/latest/features/resource_payer).

A Sample Transaction with Resource Payer:
```
{
      expiration: '2021-07-09T20:14:13.000',
      ref_block_num: 201,
      ref_block_prefix: 312235882,
      resource_payer: {
        payer: 'alice',
        max_net_bytes: 4096,
        max_cpu_us: 400,
        max_memory_bytes: 0
      },
      actions: [
        {
          account: 'eosio.token',
          name: 'transfer',
          authorization: [
            { actor: 'bob', permission: 'active' },
            { actor: 'alice', permission: 'active' }
          ],
          data: {
            from: 'bob',
            to: 'alice',
            quantity: '0.0001 SYS',
            memo: 'resource payer'
          }
        }
      ],
      transaction_extensions: [ [ 1, '0000000000855C34001000000000000090010000000000000000000000000000' ] ],
      context_free_actions: []
}
```

## Upgrading from previous versions of EOSIO

Refer to this section on the [Upgrade Guide: Upgrading from previous versions of EOSIO](../20_upgrade-guide/index.md#upgrading-from-previous-versions-of-eosio).

### Deprecation and Removal Notices

This release will remove support of MacOS 10.14 ([#10418](https://github.com/EOSIO/eos/pull/10418)) and Ubuntu16 ([#10421](https://github.com/EOSIO/eos/pull/10421)).


## Stability Updates 
- ([#9738](https://github.com/EOSIO/eos/pull/9738)) bios-boot-tutorial.py: nodeos needs more time on linux before it starts accepting p2p transactions
- ([#9742](https://github.com/EOSIO/eos/pull/9742)) When starting a replay, the rocksdb database files should be deleted.
- ([#9758](https://github.com/EOSIO/eos/pull/9758)) Consolidated Security Fixes from 2.0.8
- ([#9761](https://github.com/EOSIO/eos/pull/9761)) Fix strict aliasing violations
- ([#9810](https://github.com/EOSIO/eos/pull/9810)) Fix the truncate bug in Ship
- ([#9815](https://github.com/EOSIO/eos/pull/9815)) Fix snapshot test_compatible_versions failure and reenable it - develop
- ([#9821](https://github.com/EOSIO/eos/pull/9821)) Fix ship truncate problem with stride
- ([#9830](https://github.com/EOSIO/eos/pull/9830)) Fix Ship backward compatibility issue
- ([#9843](https://github.com/EOSIO/eos/pull/9843)) Fix packed transaction version conversion -- Develop
- ([#9877](https://github.com/EOSIO/eos/pull/9877)) DeepMind logger holds `const char*` fields that it uses for some of its naming and indicators
- ([#9865](https://github.com/EOSIO/eos/pull/9865)) fix incorrect transaction_extensions declaration
- ([#9879](https://github.com/EOSIO/eos/pull/9879)) Fix ship big vector serialization
- ([#9895](https://github.com/EOSIO/eos/pull/9895)) Fix state_history zlib_unpack bug
- ([#9910](https://github.com/EOSIO/eos/pull/9910)) fix state_history::length_writer
- ([#9923](https://github.com/EOSIO/eos/pull/9923)) get_producers unittest 
- ([#9898](https://github.com/EOSIO/eos/pull/9898)) fix build problem of non-root user using script
- ([#9960](https://github.com/EOSIO/eos/pull/9960)) Explicit ABI conversion of signed_transaction
- ([#10012](https://github.com/EOSIO/eos/pull/10012)) EPE-165: Improve logic for unlinkable blocks while sync'ing
- ([#10006](https://github.com/EOSIO/eos/pull/10006)) use p2p address for duplicate connection resolution
- ([#10061](https://github.com/EOSIO/eos/pull/10061)) fix restart without blocks.log but with retained blocks files
- ([#10084](https://github.com/EOSIO/eos/pull/10084)) Huangminghuang/merge blocklog fix
- ([#10184](https://github.com/EOSIO/eos/pull/10184)) Disable subjective tolerance when disable-subjective-billing - develop
- ([#10192](https://github.com/EOSIO/eos/pull/10192)) Merge security fix PR#365 into develop
- ([#10220](https://github.com/EOSIO/eos/pull/10220)) develop: make cleos unpack a packed transaction before signing and ad…
- ([#10284](https://github.com/EOSIO/eos/pull/10284)) huangminghuang/fix-ship-empty-block
- ([#10321](https://github.com/EOSIO/eos/pull/10321)) huangminghuang/fix-state-history
- ([#10325](https://github.com/EOSIO/eos/pull/10325)) Large ship deltas - develop
- ([#10349](https://github.com/EOSIO/eos/pull/10349)) net_plugin bugfix for privacy test corner case
- ([#10381](https://github.com/EOSIO/eos/pull/10381)) fixing flakiness in voting and forked chain tests
- ([#10390](https://github.com/EOSIO/eos/pull/10390)) net_plugin fix for privacy groups corner case

## Other Changes
- ([#9805](https://github.com/EOSIO/eos/pull/9805)) chain_plugin db intrinsic table RPC calls incorrectly handling --lower and --upper in certain scenarios
- ([#9905](https://github.com/EOSIO/eos/pull/9905)) Add log path for unsupported block log version exception
- ([#9916](https://github.com/EOSIO/eos/pull/9916)) EPE-482 Fix compiler warnings in build
- ([#9967](https://github.com/EOSIO/eos/pull/9967)) updated to new docker hub repo EOSIO instead EOS
- ([#7247](https://github.com/EOSIO/eos/pull/7247)) Change default log level from debug to info. - develop
- ([#7238](https://github.com/EOSIO/eos/pull/7238)) Remove unused cpack bits; these are not being used
- ([#7248](https://github.com/EOSIO/eos/pull/7248)) Pass env to build script installs
- ([#6913](https://github.com/EOSIO/eos/pull/6913)) Block log util#6884
- ([#7249](https://github.com/EOSIO/eos/pull/7249)) Http configurable logging
- ([#7255](https://github.com/EOSIO/eos/pull/7255)) Move Test Metrics Code to Agent
- ([#7217](https://github.com/EOSIO/eos/pull/7217)) Created Universal Pipeline Configuration File
- ([#7264](https://github.com/EOSIO/eos/pull/7264)) use protocol-features-sync-nodes branch for LRT pipeline - develop
- ([#7207](https://github.com/EOSIO/eos/pull/7207)) Optimize mongodb plugin
- ([#7276](https://github.com/EOSIO/eos/pull/7276)) correct net_plugin's win32 check
- ([#7279](https://github.com/EOSIO/eos/pull/7279)) build unittests in c++17, not c++14
- ([#7282](https://github.com/EOSIO/eos/pull/7282)) Fix cleos REX help - develop
- ([#7284](https://github.com/EOSIO/eos/pull/7284)) remove boost asio string_view workaround
- ([#7286](https://github.com/EOSIO/eos/pull/7286)) chainbase uniqueness violation fixes
- ([#7287](https://github.com/EOSIO/eos/pull/7287)) restrict range of error codes that contracts are allowed to emit
- ([#7278](https://github.com/EOSIO/eos/pull/7278)) simplify openssl setup in cmakelists
- ([#7288](https://github.com/EOSIO/eos/pull/7288)) Changes for Boost 1_70_0
- ([#7285](https://github.com/EOSIO/eos/pull/7285)) Use of Mac Anka Fleet instead of iMac fleet
- ([#7293](https://github.com/EOSIO/eos/pull/7293)) Update EosioTester.cmake.in (develop)
- ([#7290](https://github.com/EOSIO/eos/pull/7290))  Block log util test
- ([#6845](https://github.com/EOSIO/eos/pull/6845)) Net plugin multithread
- ([#7295](https://github.com/EOSIO/eos/pull/7295)) Modify the pipeline control file for triggered builds
- ([#7305](https://github.com/EOSIO/eos/pull/7305)) Eliminating trigger to allow for community PR jobs to run again
- ([#7306](https://github.com/EOSIO/eos/pull/7306)) when unable to find mongo driver stop cmake
- ([#7309](https://github.com/EOSIO/eos/pull/7309)) add debug_mode to state history plugin - develop
- ([#7310](https://github.com/EOSIO/eos/pull/7310)) Switched to git checkout of commit + supporting forked repo
- ([#7291](https://github.com/EOSIO/eos/pull/7291)) add DB guard check for nodeos_under_min_avail_ram.py
- ([#7240](https://github.com/EOSIO/eos/pull/7240)) add signal tests
- ([#7315](https://github.com/EOSIO/eos/pull/7315)) Add REX balance info to cleos get account command
- ([#7304](https://github.com/EOSIO/eos/pull/7304)) transaction_metadata thread safety
- ([#7321](https://github.com/EOSIO/eos/pull/7321)) Only call init if system contract is loaded
- ([#7275](https://github.com/EOSIO/eos/pull/7275)) remove win32 from CMakeLists.txt
- ([#7322](https://github.com/EOSIO/eos/pull/7322)) Fix under min avail ram test
- ([#7327](https://github.com/EOSIO/eos/pull/7327)) fix block serialization order in fork_database::close - develop
- ([#7331](https://github.com/EOSIO/eos/pull/7331)) Use debug level logging for --verbose
- ([#7325](https://github.com/EOSIO/eos/pull/7325)) Look in both lib and lib64 for CMake modules when building EOSIO Tester
- ([#7337](https://github.com/EOSIO/eos/pull/7337)) correct signed mismatch warning in http_plugin
- ([#7348](https://github.com/EOSIO/eos/pull/7348)) Anka develop fix
- ([#7320](https://github.com/EOSIO/eos/pull/7320)) Add test for various chainbase objects which contain fields that require dynamic allocation
- ([#7350](https://github.com/EOSIO/eos/pull/7350)) correct signed mismatch warning in chain controller
- ([#7356](https://github.com/EOSIO/eos/pull/7356)) Fixes for Boost 1.70 to compile with our current CMake
- ([#7359](https://github.com/EOSIO/eos/pull/7359)) update to use boost 1.70
- ([#7341](https://github.com/EOSIO/eos/pull/7341)) Add Version Check for Package Builder
- ([#7367](https://github.com/EOSIO/eos/pull/7367)) Fix exception #
- ([#7342](https://github.com/EOSIO/eos/pull/7342)) Update to appbase max priority on main thread
- ([#7336](https://github.com/EOSIO/eos/pull/7336)) (softfloat sync) clean up strict-aliasing rules warnings
- ([#7371](https://github.com/EOSIO/eos/pull/7371)) Adds configuration for replay test pipeline
- ([#7370](https://github.com/EOSIO/eos/pull/7370)) Fix `-b` flag for `cleos get table` subcommand
- ([#7369](https://github.com/EOSIO/eos/pull/7369)) Add `boost/asio/io_context.hpp` header to `transaction_metadata.hpp` for branch `develop`
- ([#7385](https://github.com/EOSIO/eos/pull/7385)) Ship: port #7383 and #7384 to develop
- ([#7377](https://github.com/EOSIO/eos/pull/7377)) Allow aliases of variants in ABI
- ([#7316](https://github.com/EOSIO/eos/pull/7316)) Explicit name
- ([#7389](https://github.com/EOSIO/eos/pull/7389)) Fix develop merge
- ([#7379](https://github.com/EOSIO/eos/pull/7379)) transaction deadline cleanup
- ([#7380](https://github.com/EOSIO/eos/pull/7380)) Producer incoming-transaction-queue-size-mb
- ([#7391](https://github.com/EOSIO/eos/pull/7391)) Allow for EOS clone to be a submodule
- ([#7390](https://github.com/EOSIO/eos/pull/7390)) port db_modes_test to python
- ([#7399](https://github.com/EOSIO/eos/pull/7399)) throw error if trying to create non R1 key on SE or YubiHSM wallet
- ([#7394](https://github.com/EOSIO/eos/pull/7394)) (fc sync) static_variant improvements & fix certificate trust when trust settings is empty
- ([#7405](https://github.com/EOSIO/eos/pull/7405)) Centralize EOSIO Pipeline
- ([#7401](https://github.com/EOSIO/eos/pull/7401)) state database versioning
- ([#7392](https://github.com/EOSIO/eos/pull/7392)) Trx blk connections
- ([#7433](https://github.com/EOSIO/eos/pull/7433)) use imported targets for boost & cleanup fc/appbase/chainbase standalone usage
- ([#7434](https://github.com/EOSIO/eos/pull/7434)) (chainbase) don’t keep file mapping active when in heap/locked mode
- ([#7432](https://github.com/EOSIO/eos/pull/7432)) No need to start keosd for cleos command which doesn't need keosd
- ([#7430](https://github.com/EOSIO/eos/pull/7430)) Add option for cleos sign subcommand to ask keosd for signing
- ([#7425](https://github.com/EOSIO/eos/pull/7425)) txn json to file
- ([#7440](https://github.com/EOSIO/eos/pull/7440)) Fix #7436 SIGSEGV - develop
- ([#7461](https://github.com/EOSIO/eos/pull/7461)) Name txn_test_gen threads
- ([#7442](https://github.com/EOSIO/eos/pull/7442)) Enhance cleos error message when parsing JSON argument
- ([#7451](https://github.com/EOSIO/eos/pull/7451)) set initial costs for expensive parallel unit tests
- ([#7366](https://github.com/EOSIO/eos/pull/7366)) BATS bash tests for build scripts + various other improvements and fixes
- ([#7467](https://github.com/EOSIO/eos/pull/7467)) Pipeline Configuration File Update
- ([#7476](https://github.com/EOSIO/eos/pull/7476)) Various improvements from pull/7458
- ([#7488](https://github.com/EOSIO/eos/pull/7488)) Various BATS fixes to fix CI/CD
- ([#7489](https://github.com/EOSIO/eos/pull/7489)) Fix Incorrectly Resolved and Untested Merge Conflicts in Pipeline Configuration File
- ([#7474](https://github.com/EOSIO/eos/pull/7474)) fix copy_bin() for win32 builds
- ([#7478](https://github.com/EOSIO/eos/pull/7478)) remove stray SIGUSR1
- ([#7475](https://github.com/EOSIO/eos/pull/7475)) guard unix socket support in http_plugin depending on platform support
- ([#7492](https://github.com/EOSIO/eos/pull/7492)) Enabled helpers for unpinned builds.
- ([#7482](https://github.com/EOSIO/eos/pull/7482)) Let delete-all-blocks option to only remove the contents instead of the directory itself
- ([#7502](https://github.com/EOSIO/eos/pull/7502)) [develop] Versioned images (prep for hashing)
- ([#7507](https://github.com/EOSIO/eos/pull/7507)) use create_directories in initialize_protocol_features - develop
- ([#7484](https://github.com/EOSIO/eos/pull/7484)) return zero exit status for nodeos version, help, fixed reversible, and extracted genesis
- ([#7468](https://github.com/EOSIO/eos/pull/7468)) Versioning library
- ([#7486](https://github.com/EOSIO/eos/pull/7486)) [develop] Ensure we're in repo root
- ([#7515](https://github.com/EOSIO/eos/pull/7515)) Custom path support for eosio installation.
- ([#7516](https://github.com/EOSIO/eos/pull/7516)) on supported platforms build with system clang by default
- ([#7519](https://github.com/EOSIO/eos/pull/7519)) [develop] Removed lrt from pipeline.jsonc
- ([#7525](https://github.com/EOSIO/eos/pull/7525)) [develop] Readlink quick fix and BATS test fixes
- ([#7532](https://github.com/EOSIO/eos/pull/7532)) [develop] BASE IMAGE Fixes
- ([#7535](https://github.com/EOSIO/eos/pull/7535)) Add -j option to print JSON format for cleos get currency balance.
- ([#7537](https://github.com/EOSIO/eos/pull/7537)) connection via listen needs to start in connecting mode
- ([#7497](https://github.com/EOSIO/eos/pull/7497)) properly add single quotes for parameter with spaces in logs output
- ([#7544](https://github.com/EOSIO/eos/pull/7544)) restore usage of devtoolset-8 on centos7
- ([#7547](https://github.com/EOSIO/eos/pull/7547)) Sighup logging
- ([#7421](https://github.com/EOSIO/eos/pull/7421)) WebAuthn key and signature support
- ([#7572](https://github.com/EOSIO/eos/pull/7572)) [develop] Don't create mongo folders unless ENABLE_MONGO is true
- ([#7576](https://github.com/EOSIO/eos/pull/7576)) [develop] Better found/not found messages for clarity
- ([#7545](https://github.com/EOSIO/eos/pull/7545)) add nodiscard attribute to tester's push_action
- ([#7581](https://github.com/EOSIO/eos/pull/7581)) Update to fc with logger fix
- ([#7558](https://github.com/EOSIO/eos/pull/7558)) [develop] Ability to set *_DIR on CLI
- ([#7553](https://github.com/EOSIO/eos/pull/7553)) [develop] CMAKE version check before dependencies are installed
- ([#7584](https://github.com/EOSIO/eos/pull/7584)) fix hash<>::result_type deprecation spam
- ([#7588](https://github.com/EOSIO/eos/pull/7588)) [develop] Various BATS test fixes
- ([#7578](https://github.com/EOSIO/eos/pull/7578)) [develop] -i support for relative paths
- ([#7449](https://github.com/EOSIO/eos/pull/7449)) add accurate checktime timer for macOS
- ([#7591](https://github.com/EOSIO/eos/pull/7591)) [develop] SUDO_COMMAND -> NEW_SUDO_COMMAND (base fixed)
- ([#7594](https://github.com/EOSIO/eos/pull/7594)) [develop] Install location fix
- ([#7586](https://github.com/EOSIO/eos/pull/7586)) Modify transaction_ack to process bcast_transaction and rejected_transaction correctly
- ([#7585](https://github.com/EOSIO/eos/pull/7585)) Enhance cleos to enable new RPC send_transaction
- ([#7599](https://github.com/EOSIO/eos/pull/7599)) Use updated sync nodes for sync tests
- ([#7608](https://github.com/EOSIO/eos/pull/7608)) indicate in brew bottle mojave is required
- ([#7615](https://github.com/EOSIO/eos/pull/7615)) Change hardcoded currency symbol in testnet.template into a variable
- ([#7610](https://github.com/EOSIO/eos/pull/7610)) use explicit billing for unapplied and deferred transactions in tester - develop
- ([#7621](https://github.com/EOSIO/eos/pull/7621)) Call resolve on connection strand
- ([#7623](https://github.com/EOSIO/eos/pull/7623)) additional wasm unittests around max depth
- ([#7607](https://github.com/EOSIO/eos/pull/7607)) Improve nodeos make-index speeds
- ([#7598](https://github.com/EOSIO/eos/pull/7598)) Net plugin block id notification
- ([#7624](https://github.com/EOSIO/eos/pull/7624)) bios-boot-tutorial.py: bugfix, SYS hardcoded instead of using command…
- ([#7632](https://github.com/EOSIO/eos/pull/7632)) [develop] issues/7627: Install script missing !
- ([#7634](https://github.com/EOSIO/eos/pull/7634)) fix fc::temp_directory usage in tester
- ([#7642](https://github.com/EOSIO/eos/pull/7642)) remove stale warning about dirty metadata
- ([#7640](https://github.com/EOSIO/eos/pull/7640)) fix win32 build of eosio-blocklog
- ([#7643](https://github.com/EOSIO/eos/pull/7643)) remove unused dlfcn.h include; troublesome for win32
- ([#7645](https://github.com/EOSIO/eos/pull/7645)) add eosio-blocklog to base install component
- ([#7651](https://github.com/EOSIO/eos/pull/7651)) Port #7619 to develop
- ([#7652](https://github.com/EOSIO/eos/pull/7652)) remove raise() in keosd in favor of simple appbase quit (de-posix it)
- ([#7453](https://github.com/EOSIO/eos/pull/7453)) Refactor unapplied transaction queue
- ([#7657](https://github.com/EOSIO/eos/pull/7657)) (chainbase sync) print name of DB causing failure condition & win32 fixes
- ([#7663](https://github.com/EOSIO/eos/pull/7663)) Fix path error in cleos set code/abi
- ([#7633](https://github.com/EOSIO/eos/pull/7633)) Small optimization to move more trx processing off application thread
- ([#7662](https://github.com/EOSIO/eos/pull/7662)) fix fork resolve in special case
- ([#7667](https://github.com/EOSIO/eos/pull/7667)) fix 7600 double confirm after changing sign key
- ([#7625](https://github.com/EOSIO/eos/pull/7625)) Fix flaky tests - mainly net_plugin fixes
- ([#7672](https://github.com/EOSIO/eos/pull/7672)) Fix memory leak
- ([#7676](https://github.com/EOSIO/eos/pull/7676)) Remove unused code
- ([#7677](https://github.com/EOSIO/eos/pull/7677)) Commas go outside the quotes...
- ([#7675](https://github.com/EOSIO/eos/pull/7675)) wasm unit test with an imported function as start function
- ([#7678](https://github.com/EOSIO/eos/pull/7678)) Issue 3516 fix
- ([#7404](https://github.com/EOSIO/eos/pull/7404)) wtmsig block production
- ([#7686](https://github.com/EOSIO/eos/pull/7686)) Unapplied transaction queue performance
- ([#7685](https://github.com/EOSIO/eos/pull/7685)) Integration Test descriptions and timeout fix
- ([#7691](https://github.com/EOSIO/eos/pull/7691)) Fix nodeos 1.8.x to > 1.7.x peering issue (allowed-connection not equal to "any")
- ([#7250](https://github.com/EOSIO/eos/pull/7250)) wabt: reduce redundant memset
- ([#7702](https://github.com/EOSIO/eos/pull/7702)) fix producer_plugin watermark tracking - develop
- ([#7477](https://github.com/EOSIO/eos/pull/7477)) Fix abi_serializer to encode optional non-built_in types
- ([#7703](https://github.com/EOSIO/eos/pull/7703)) return flat_multimap from transaction::validate_and_extract_extensions
- ([#7720](https://github.com/EOSIO/eos/pull/7720)) Fix bug to make sed -i work properly on Mac
- ([#7716](https://github.com/EOSIO/eos/pull/7716)) [TRAVIS POC] develop Support passing in JOBS for docker/kube multi-tenancy
- ([#7707](https://github.com/EOSIO/eos/pull/7707)) add softfloat only injection mode
- ([#7725](https://github.com/EOSIO/eos/pull/7725)) Fix increment in test
- ([#7729](https://github.com/EOSIO/eos/pull/7729)) Fix db_modes_test
- ([#7734](https://github.com/EOSIO/eos/pull/7734)) Fix db exhaustion
- ([#7487](https://github.com/EOSIO/eos/pull/7487)) Enable extended_asset to be encoded from array
- ([#7736](https://github.com/EOSIO/eos/pull/7736)) unit test ensuring that OOB table init allowed on set code; fails on action
- ([#7744](https://github.com/EOSIO/eos/pull/7744)) (appbase) update to get non-option fix & unique_ptr tweak
- ([#7746](https://github.com/EOSIO/eos/pull/7746)) (chainbase sync) fix build with boost 1.71
- ([#7721](https://github.com/EOSIO/eos/pull/7721)) Improve signature recovery
- ([#7757](https://github.com/EOSIO/eos/pull/7757)) remove stale license headers
- ([#7756](https://github.com/EOSIO/eos/pull/7756)) block_log performance improvement, and misc.
- ([#7654](https://github.com/EOSIO/eos/pull/7654)) exclusively use timer for checktime
- ([#7763](https://github.com/EOSIO/eos/pull/7763)) Use fc::cfile instead of std::fstream for state_history
- ([#7770](https://github.com/EOSIO/eos/pull/7770)) Net plugin sync fix
- ([#7717](https://github.com/EOSIO/eos/pull/7717)) Support for v2 snapshots with pending producer schedules
- ([#7795](https://github.com/EOSIO/eos/pull/7795)) Hardcode initial eosio ABI: #7794
- ([#7792](https://github.com/EOSIO/eos/pull/7792)) Restore default logging if logging.json is removed when SIGHUP.
- ([#7791](https://github.com/EOSIO/eos/pull/7791)) Producer plugin
- ([#7786](https://github.com/EOSIO/eos/pull/7786)) Remove redundant work from net plugin
- ([#7785](https://github.com/EOSIO/eos/pull/7785)) Optimize block log usage
- ([#7812](https://github.com/EOSIO/eos/pull/7812)) Add IMPORTANT file and update README - develop
- ([#7820](https://github.com/EOSIO/eos/pull/7820)) rename IMPORTANT to IMPORTANT.md - develop
- ([#7809](https://github.com/EOSIO/eos/pull/7809)) Correct cpu_usage calculation when more than one signature
- ([#7803](https://github.com/EOSIO/eos/pull/7803)) callback support for checktime timer expiry
- ([#7838](https://github.com/EOSIO/eos/pull/7838)) Bandwidth - develop
- ([#7845](https://github.com/EOSIO/eos/pull/7845)) Deprecate network version match - develop
- ([#7700](https://github.com/EOSIO/eos/pull/7700)) [develop] Travis CI + Buildkite 3.0
- ([#7825](https://github.com/EOSIO/eos/pull/7825)) apply_block optimization
- ([#7849](https://github.com/EOSIO/eos/pull/7849)) Increase Contracts Builder Timeout + Fix $SKIP_MAC
- ([#7854](https://github.com/EOSIO/eos/pull/7854)) Net plugin sync
- ([#7860](https://github.com/EOSIO/eos/pull/7860)) [develop] Ensure release flag is added to all builds.
- ([#7873](https://github.com/EOSIO/eos/pull/7873)) [develop] Mac Builder Boost Fix
- ([#7868](https://github.com/EOSIO/eos/pull/7868)) cleos get actions
- ([#7774](https://github.com/EOSIO/eos/pull/7774)) update WAVM to be compatible with LLVM 7 through 9
- ([#7864](https://github.com/EOSIO/eos/pull/7864)) Add output of build info on nodeos startup
- ([#7881](https://github.com/EOSIO/eos/pull/7881)) [develop] Ensure Artfacts Upload on Failed Tests
- ([#7886](https://github.com/EOSIO/eos/pull/7886)) promote read-only disablement log from net_plugin to warn level
- ([#7887](https://github.com/EOSIO/eos/pull/7887)) print unix socket path when there is an error starting unix socket server
- ([#7889](https://github.com/EOSIO/eos/pull/7889)) Update docker builder tag
- ([#7891](https://github.com/EOSIO/eos/pull/7891)) Fix exit crash - develop
- ([#7877](https://github.com/EOSIO/eos/pull/7877)) Create Release Build Test
- ([#7883](https://github.com/EOSIO/eos/pull/7883)) Add Support for eosio-test-stability Pipeline
- ([#7903](https://github.com/EOSIO/eos/pull/7903)) adds support for builder priority queues
- ([#7853](https://github.com/EOSIO/eos/pull/7853)) change behavior of recover_key to better support variable length keys
- ([#7901](https://github.com/EOSIO/eos/pull/7901)) [develop] Add Trigger for LRTs and Multiversion Tests Post PR
- ([#7914](https://github.com/EOSIO/eos/pull/7914)) [Develop] Forked PR fix
- ([#7923](https://github.com/EOSIO/eos/pull/7923)) return error when attempting to remove key from YubiHSM wallet
- ([#7910](https://github.com/EOSIO/eos/pull/7910)) [Develop] Updated anka plugin, added failover for registries, and added sleep fix for git clone/networking bug
- ([#7926](https://github.com/EOSIO/eos/pull/7926)) Better error check in test
- ([#7919](https://github.com/EOSIO/eos/pull/7919)) decouple wavm runtime from being required & initial support for platform specific wasm runtimes
- ([#7927](https://github.com/EOSIO/eos/pull/7927)) Fix Release Build Type for macOS on Travis CI
- ([#7931](https://github.com/EOSIO/eos/pull/7931)) Fix intermittent crash on exit when port already in use - develop
- ([#7930](https://github.com/EOSIO/eos/pull/7930)) use -fdiagnostics-color=always even for clang
- ([#7933](https://github.com/EOSIO/eos/pull/7933)) [Develop] Support all BK/Travis cases in Submodule Regression Script
- ([#7943](https://github.com/EOSIO/eos/pull/7943)) Change eosio-launcher enable-gelf-logging argument default to false.
- ([#7946](https://github.com/EOSIO/eos/pull/7946)) Forked chain test error statement - develop
- ([#7948](https://github.com/EOSIO/eos/pull/7948)) net_plugin correctly handle unknown_block_exception - develop
- ([#7954](https://github.com/EOSIO/eos/pull/7954)) remove bad semicolon (in unused but compiled code)
- ([#7952](https://github.com/EOSIO/eos/pull/7952)) Unable to Create Block Log Index #7865
- ([#7953](https://github.com/EOSIO/eos/pull/7953)) Refactor producer plugin start_block - develop
- ([#7841](https://github.com/EOSIO/eos/pull/7841)) 7646 chain id in blog
- ([#7958](https://github.com/EOSIO/eos/pull/7958)) [develop] Fix Mac builds on Travis
- ([#7962](https://github.com/EOSIO/eos/pull/7962)) set immutable chain_id during construction of controller
- ([#7957](https://github.com/EOSIO/eos/pull/7957)) Upgrade to Boost 1.71.0
- ([#7971](https://github.com/EOSIO/eos/pull/7971)) Net plugin unexpected block - develop
- ([#7967](https://github.com/EOSIO/eos/pull/7967)) support unix socket HTTP server for nodeos
- ([#7947](https://github.com/EOSIO/eos/pull/7947)) Function body code size test
- ([#7955](https://github.com/EOSIO/eos/pull/7955)) EOSIO WASM Spec tests
- ([#7978](https://github.com/EOSIO/eos/pull/7978)) use the LLVM 7 library provided by SCL on CentOS7
- ([#7530](https://github.com/EOSIO/eos/pull/7530)) Add more2 to get_table_rows API
- ([#7983](https://github.com/EOSIO/eos/pull/7983)) port consolidated security fixes for 1.8.4 to develop; add unit tests associated with consolidated security fixes for 1.8.1
- ([#7986](https://github.com/EOSIO/eos/pull/7986)) Remove unnecessary comment
- ([#7985](https://github.com/EOSIO/eos/pull/7985)) more bug fixes with chain_id in state changes
- ([#7974](https://github.com/EOSIO/eos/pull/7974)) Experimental/wb2 jit
- ([#7989](https://github.com/EOSIO/eos/pull/7989)) Correct designator order for field of get_table_rows_params
- ([#7992](https://github.com/EOSIO/eos/pull/7992)) move wasm_allocator from wasm_interface to controller
- ([#7995](https://github.com/EOSIO/eos/pull/7995)) new timeout to handle when two jobs on the same host are maxing their…
- ([#7993](https://github.com/EOSIO/eos/pull/7993)) update eos-vm to latest develop, fix issues with instantiation limit …
- ([#7991](https://github.com/EOSIO/eos/pull/7991)) missing block log chain id unit tests
- ([#8002](https://github.com/EOSIO/eos/pull/8002)) Ensure block signing authority does not have a threshold of 0
- ([#8001](https://github.com/EOSIO/eos/pull/8001)) Net plugin trx progress - develop
- ([#8003](https://github.com/EOSIO/eos/pull/8003)) update eos-vm ref
- ([#7988](https://github.com/EOSIO/eos/pull/7988)) Net plugin version match
- ([#7975](https://github.com/EOSIO/eos/pull/7975)) EOS-VM Optimized Compiler
- ([#8007](https://github.com/EOSIO/eos/pull/8007)) disallow WAVM with EOS-VM OC
- ([#8010](https://github.com/EOSIO/eos/pull/8010)) Change log level of index write
- ([#8009](https://github.com/EOSIO/eos/pull/8009)) pending incoming order on subjective failure
- ([#8013](https://github.com/EOSIO/eos/pull/8013)) ensure eos-vm-oc headers get installed
- ([#8008](https://github.com/EOSIO/eos/pull/8008)) Increase stability of nodeos_under_min_avail_ram.py - develop
- ([#8015](https://github.com/EOSIO/eos/pull/8015)) two fixes for eosio tester cmake modules
- ([#8019](https://github.com/EOSIO/eos/pull/8019)) [Develop] Change submodule script to see stderr for git commands
- ([#8021](https://github.com/EOSIO/eos/pull/8021)) Avoid validating keys within block signing authorities in proposed schedule
- ([#8014](https://github.com/EOSIO/eos/pull/8014)) Retain persisted trx until expired on speculative nodes
- ([#8024](https://github.com/EOSIO/eos/pull/8024)) Add optional ability to disable WASM Spec Tests
- ([#8023](https://github.com/EOSIO/eos/pull/8023)) Make subjective_cpu_leeway a config option
- ([#8025](https://github.com/EOSIO/eos/pull/8025)) Fix build script LLVM symlinking
- ([#8026](https://github.com/EOSIO/eos/pull/8026)) update EOS VM Optimized Compiler naming convention
- ([#8012](https://github.com/EOSIO/eos/pull/8012)) 7939 trim block log v3 support
- ([#8029](https://github.com/EOSIO/eos/pull/8029)) update eos-vm ref and install eos-vm license
- ([#8033](https://github.com/EOSIO/eos/pull/8033)) net_plugin better error for unknown block
- ([#8034](https://github.com/EOSIO/eos/pull/8034)) EOS VM OC license updates
- ([#8181](https://github.com/EOSIO/eos/pull/8181)) Chainbase rewrite
- ([#9073](https://github.com/EOSIO/eos/pull/9073)) Deep Mind off-chain ABI serializer & FC encoded hex output
- ([#9732](https://github.com/EOSIO/eos/pull/9732)) Fix unresolved set_kv_parameters_packed problem
- ([#9723](https://github.com/EOSIO/eos/pull/9723)) Remove compiler warnings from test_kv_rocksdb.cpp
- ([#9734](https://github.com/EOSIO/eos/pull/9734)) update bios to have 'temporary' setpparams action
- ([#9739](https://github.com/EOSIO/eos/pull/9739)) Fixing blockvault test for macos 10.14
- ([#9735](https://github.com/EOSIO/eos/pull/9735)) Adding enable-kv.sh script
- ([#9747](https://github.com/EOSIO/eos/pull/9747)) Prune test diagnostics
- ([#9750](https://github.com/EOSIO/eos/pull/9750)) Fix PostgreSQL transaction problem
- ([#9749](https://github.com/EOSIO/eos/pull/9749)) Added a new application method for sync from blockvault
- ([#9751](https://github.com/EOSIO/eos/pull/9751)) blockvault_client_plugin logger
- ([#9743](https://github.com/EOSIO/eos/pull/9743)) fixed get_kv_table_rows issues
- ([#9744](https://github.com/EOSIO/eos/pull/9744)) Fix backing store switching issues
- ([#9740](https://github.com/EOSIO/eos/pull/9740)) improve EOS VM OC cache lookup performance via hashed index
- ([#9752](https://github.com/EOSIO/eos/pull/9752)) sync from block vault during block production
- ([#9714](https://github.com/EOSIO/eos/pull/9714)) Remove unused code
- ([#9760](https://github.com/EOSIO/eos/pull/9760)) Optimizations
- ([#9726](https://github.com/EOSIO/eos/pull/9726)) adjust validate-reflection-test to work with any directory layout
- ([#9762](https://github.com/EOSIO/eos/pull/9762)) Added handling of duplicate reversible blocks.
- ([#9706](https://github.com/EOSIO/eos/pull/9706)) Default to returning WASM as returning WAST has been deprecated.
- ([#9766](https://github.com/EOSIO/eos/pull/9766)) update dependencies for deb and rpm packages
- ([#9767](https://github.com/EOSIO/eos/pull/9767)) Fix compiler warnings
- ([#9768](https://github.com/EOSIO/eos/pull/9768)) FIX libpqxx not found problem
- ([#9769](https://github.com/EOSIO/eos/pull/9769)) Version number changes in README.md for enable-kv.sh script
- ([#9770](https://github.com/EOSIO/eos/pull/9770)) Workaround the PowerTools name change
- ([#9772](https://github.com/EOSIO/eos/pull/9772)) Saving and restoring undo stack at shutdown and startup
- ([#9777](https://github.com/EOSIO/eos/pull/9777)) Add feature BLOCKCHAIN_PARAMETERS
- ([#9782](https://github.com/EOSIO/eos/pull/9782)) Update supported operating systems in README.md
- ([#9787](https://github.com/EOSIO/eos/pull/9787)) Fix build script problem with older version of cmake
- ([#9776](https://github.com/EOSIO/eos/pull/9776)) Add CentOS 8 Package Builder Step
- ([#9790](https://github.com/EOSIO/eos/pull/9790)) fix unstable nodeos_run_test.py 
- ([#9778](https://github.com/EOSIO/eos/pull/9778)) Reduce logging for failed http plugin calls
- ([#9826](https://github.com/EOSIO/eos/pull/9826)) Add warning interval option for resource monitor plugin
- ([#9771](https://github.com/EOSIO/eos/pull/9771)) Fix libpqxx warnings on Mac
- ([#9724](https://github.com/EOSIO/eos/pull/9724)) Enable test test_state_history::test_deltas_contract_several_rows
- ([#9814](https://github.com/EOSIO/eos/pull/9814)) Re-enable full multi_index_tests
- ([#9837](https://github.com/EOSIO/eos/pull/9837)) fix populating some information for get account
- ([#9846](https://github.com/EOSIO/eos/pull/9846)) Add easter egg for release 2.1-rc2
- ([#9859](https://github.com/EOSIO/eos/pull/9859)) Fix problem when using ubuntu libpqxx package
- ([#9831](https://github.com/EOSIO/eos/pull/9831)) Fix or inhibit various compiler warnings while building on clang.
- ([#9874](https://github.com/EOSIO/eos/pull/9874)) Fix  build problem on cmake 3.10
- ([#9881](https://github.com/EOSIO/eos/pull/9881)) Removed some unnecessary cmake commands as they get inherited from the parent cmake.
- ([#9883](https://github.com/EOSIO/eos/pull/9883)) Fix problem with libpqxx 7.3.0 upgrade
- ([#9888](https://github.com/EOSIO/eos/pull/9888)) Egonz/auto 313 final
- ([#9870](https://github.com/EOSIO/eos/pull/9870)) Consolidated Security Fixes for 2.0.9 - develop
- ([#9917](https://github.com/EOSIO/eos/pull/9917)) Create a fix to support requested encoding type in next_key
- ([#9918](https://github.com/EOSIO/eos/pull/9918)) Update abieos submodule - opaque type changes
- ([#9927](https://github.com/EOSIO/eos/pull/9927)) trace history log messages should print nicely in syslog
- ([#9935](https://github.com/EOSIO/eos/pull/9935)) Fix "cleos net peers" command error
- ([#9936](https://github.com/EOSIO/eos/pull/9936)) Update abieos submodule
- ([#9937](https://github.com/EOSIO/eos/pull/9937)) fix intermittent fork test failure in develop
- ([#9955](https://github.com/EOSIO/eos/pull/9955)) PowerTools is now powertools in CentOS 8.3
- ([#9962](https://github.com/EOSIO/eos/pull/9962)) Make get_kv_table_rows work without --lower and --upper and --index
- ([#9899](https://github.com/EOSIO/eos/pull/9899)) use oob cmake if possible so as to save script building time
- ([#9915](https://github.com/EOSIO/eos/pull/9915)) extra transaction data integration test
- ([#9889](https://github.com/EOSIO/eos/pull/9889)) EOS VM OC: Support LLVM 11
- ([#9885](https://github.com/EOSIO/eos/pull/9885)) replace old libsecp256k1 fork with upstream; boosts k1 recovery performance
- ([#9973](https://github.com/EOSIO/eos/pull/9973)) update fc
- ([#9940](https://github.com/EOSIO/eos/pull/9940)) Create eosio-debug-build Pipeline
- ([#9964](https://github.com/EOSIO/eos/pull/9964)) refactor get_kv_table_rows
- ([#9978](https://github.com/EOSIO/eos/pull/9978)) move fc::blob to/from_variant out of libchain to fc
- ([#9974](https://github.com/EOSIO/eos/pull/9974)) develop -- Fix incorrect merge of range fix in fork test from Rel 2.1
- ([#9984](https://github.com/EOSIO/eos/pull/9984)) EPE-389 Fix net_plugin stall during head cachup - merge develop
- ([#9987](https://github.com/EOSIO/eos/pull/9987)) Develop: add a second account to exhausive_snapshot_test
- ([#9919](https://github.com/EOSIO/eos/pull/9919)) Remove nonfunctional ENABLE_BLOCK_VAULT_CLIENT cmake option.
- ([#9982](https://github.com/EOSIO/eos/pull/9982)) Add unit tests for new fields added for get account in PR#9838
- ([#10013](https://github.com/EOSIO/eos/pull/10013)) [develop] Fix LRT triggers
- ([#10032](https://github.com/EOSIO/eos/pull/10032)) [develop] Fix MacOS base image failures
- ([#10044](https://github.com/EOSIO/eos/pull/10044)) Reduce Docker Hub Manifest Queries
- ([#10036](https://github.com/EOSIO/eos/pull/10036)) Add cleos system activate subcommand (to activate system feature like kv_database)
- ([#10041](https://github.com/EOSIO/eos/pull/10041)) [develop] Updated Mojave libpqxx dependency
- ([#10038](https://github.com/EOSIO/eos/pull/10038)) Updating develop to latest eos-vm eosio hash.
- ([#10039](https://github.com/EOSIO/eos/pull/10039)) Replacing db intrinsic uses of RocksDB lower_bound when direct read could be used
- ([#10037](https://github.com/EOSIO/eos/pull/10037)) implement splitting/extracting block log for eosio_blocklog
- ([#10053](https://github.com/EOSIO/eos/pull/10053)) [develop] Add Big Sur instances to CICD
- ([#10055](https://github.com/EOSIO/eos/pull/10055)) [develop] Revert Anka plugin version
- ([#10045](https://github.com/EOSIO/eos/pull/10045)) Fix multiversion test failure
- ([#10064](https://github.com/EOSIO/eos/pull/10064)) [develop] Fix docker steps on tagged builds.
- ([#10059](https://github.com/EOSIO/eos/pull/10059)) Cleos print error message from server when parse json failed
- ([#10056](https://github.com/EOSIO/eos/pull/10056)) add docker-compose based multiversion test
- ([#9966](https://github.com/EOSIO/eos/pull/9966)) add a dockerignore file ignoring .git directory
- ([#10019](https://github.com/EOSIO/eos/pull/10019)) revert changes to empty string as present for lower_bound, upper_bound, or index_value
- ([#10040](https://github.com/EOSIO/eos/pull/10040)) remove BOOST_ASIO_HAS_LOCAL_SOCKETS ifdefs
- ([#10078](https://github.com/EOSIO/eos/pull/10078)) add merge blocklog feature
- ([#10081](https://github.com/EOSIO/eos/pull/10081)) update fc
- ([#10086](https://github.com/EOSIO/eos/pull/10086)) allow fix_irreversible_block to without index file
- ([#9922](https://github.com/EOSIO/eos/pull/9922)) Remove reversible db
- ([#10087](https://github.com/EOSIO/eos/pull/10087)) add first_block_num for get_info command
- ([#10051](https://github.com/EOSIO/eos/pull/10051)) Add new loggers for dumping transaction trace
- ([#10095](https://github.com/EOSIO/eos/pull/10095)) abieos with to/from_json fix for SHiP transaction_trace
- ([#10103](https://github.com/EOSIO/eos/pull/10103)) plugin_http_api_test - fix timing issue
- ([#10100](https://github.com/EOSIO/eos/pull/10100)) Standardize JSON formatting of action data (epe 151)
- ([#10120](https://github.com/EOSIO/eos/pull/10120)) Adding test for nodeos --full-version check.
- ([#10126](https://github.com/EOSIO/eos/pull/10126)) Update eosio-resume-from-state Documentation
- ([#10125](https://github.com/EOSIO/eos/pull/10125)) fix compiling with boost 1.76; add <set> include in chainbase
- ([#10144](https://github.com/EOSIO/eos/pull/10144)) huangminghuang/kv-get-row-time-tracking
- ([#10152](https://github.com/EOSIO/eos/pull/10152)) huangminghuang/docker-compose-debug
- ([#10149](https://github.com/EOSIO/eos/pull/10149)) [develop] Update timeouts for Mac builds.
- ([#10093](https://github.com/EOSIO/eos/pull/10093)) cache key values in iterator store in db_context_rocksdb
- ([#10157](https://github.com/EOSIO/eos/pull/10157)) [develop] Improve timeouts occurring on Anka builds for remaining pipelines.
- ([#10160](https://github.com/EOSIO/eos/pull/10160)) Updating nodeos parameter tests to use cmake locations.
- ([#10168](https://github.com/EOSIO/eos/pull/10168)) Subjective bill trx - develop
- ([#10181](https://github.com/EOSIO/eos/pull/10181)) Remove cerr - develop
- ([#10173](https://github.com/EOSIO/eos/pull/10173)) develop: Exit nodeos if fork_db is corrupted and lib cannot advance
- ([#10224](https://github.com/EOSIO/eos/pull/10224)) Add test case for decompress zlib funciton of transaction, see EPE70
- ([#10231](https://github.com/EOSIO/eos/pull/10231)) Support Running Version Tests on Fresh OS Installs
- ([#10236](https://github.com/EOSIO/eos/pull/10236)) disable appbase's automatic version discovery via git
- ([#10237](https://github.com/EOSIO/eos/pull/10237)) fix db_modes_test pipeline fails, see epe889
- ([#10262](https://github.com/EOSIO/eos/pull/10262)) Correct fix-irreversible-blocks option default value which was set in…
- ([#10261](https://github.com/EOSIO/eos/pull/10261)) ship-v2
- ([#10270](https://github.com/EOSIO/eos/pull/10270)) fix compile with gcc11
- ([#10272](https://github.com/EOSIO/eos/pull/10272)) Consolidated Security Updates for 2.0.12 - develop
- ([#10278](https://github.com/EOSIO/eos/pull/10278)) SHiP new unit test - delta generation using eosio.token actions
- ([#10276](https://github.com/EOSIO/eos/pull/10276)) Ensure container.size() > 0 before initiating RocksDB writes/deletes
- ([#10293](https://github.com/EOSIO/eos/pull/10293)) Merge back submodule chainbase pull 67 to eos develolp
- ([#10279](https://github.com/EOSIO/eos/pull/10279)) Fix issue with account query db for develop
- ([#10109](https://github.com/EOSIO/eos/pull/10109)) Change blockvault client plugin initialization log message
- ([#10298](https://github.com/EOSIO/eos/pull/10298)) don't specify default values for reversible blocks db configs
- ([#10306](https://github.com/EOSIO/eos/pull/10306)) remove reversible-blocks configs from db_modes_test
- ([#10134](https://github.com/EOSIO/eos/pull/10134)) add --no-auto-keosd to cli_test
- ([#10246](https://github.com/EOSIO/eos/pull/10246)) update boost tarball location
- ([#10313](https://github.com/EOSIO/eos/pull/10313)) [develop] Add annotate in pipeline runs.
- ([#10301](https://github.com/EOSIO/eos/pull/10301)) huangminghuang/update-abios
- ([#10318](https://github.com/EOSIO/eos/pull/10318)) Removed transaction_hook additions from global_property_v2 in abieos.
- ([#10297](https://github.com/EOSIO/eos/pull/10297)) EPE-412: use better error message when a port is taken
- ([#10326](https://github.com/EOSIO/eos/pull/10326)) remove some left over "Intl" cmake variables
- ([#10319](https://github.com/EOSIO/eos/pull/10319)) Update state_history_tests.cpp
- ([#10320](https://github.com/EOSIO/eos/pull/10320)) Added  Privacy Test Case #4
- ([#10329](https://github.com/EOSIO/eos/pull/10329)) Initializing ssl_enabled.
- ([#10331](https://github.com/EOSIO/eos/pull/10331)) EPE897:Wrong cleos version test in eos/tests/
- ([#10333](https://github.com/EOSIO/eos/pull/10333)) As the wait return code is likely to be $NODEOS_PID (stopped child re…
- ([#10335](https://github.com/EOSIO/eos/pull/10335)) huangminghuang/fix-ship-block
- ([#10339](https://github.com/EOSIO/eos/pull/10339)) state-history-exit-on-write-failure
- ([#10305](https://github.com/EOSIO/eos/pull/10305)) fix boost unit test linkage; fixes build on Fedora 34
- ([#10310](https://github.com/EOSIO/eos/pull/10310)) remove old outdated MSVC bits in cmake files
- ([#10328](https://github.com/EOSIO/eos/pull/10328)) scenario 3 test added to feature privacy tests
- ([#10348](https://github.com/EOSIO/eos/pull/10348)) Removed delay on start with TLS connections
- ([#10344](https://github.com/EOSIO/eos/pull/10344)) Fix incorrect prune state of a signed_block issue
- ([#10356](https://github.com/EOSIO/eos/pull/10356)) flakiness fix for forked chain test
- ([#10363](https://github.com/EOSIO/eos/pull/10363)) to_json: use letter escapes
- ([#10376](https://github.com/EOSIO/eos/pull/10376)) Update EOS license year 2021 - develop
- ([#10372](https://github.com/EOSIO/eos/pull/10372)) Sanitize Branch Names for Docker Containers
- ([#10384](https://github.com/EOSIO/eos/pull/10384)) Copy RocksDB license into EOS repo
- ([#10387](https://github.com/EOSIO/eos/pull/10387)) Update license of submodule eosio-wasm-spec-tests to year 2021 - develop
- ([#10392](https://github.com/EOSIO/eos/pull/10392)) Addressing identified security vulnerability (node-fetch)
- ([#10405](https://github.com/EOSIO/eos/pull/10405)) test for existence of "data" field when printing action result, introduced by PR #10100
- ([#10413](https://github.com/EOSIO/eos/pull/10413)) Ro transactions remove account param
- ([#10350](https://github.com/EOSIO/eos/pull/10350)) Keep all block logs in retained directory
- ([#10377](https://github.com/EOSIO/eos/pull/10377)) Refactor block tracking to avoid re-sending to sender - develop
- ([#10414](https://github.com/EOSIO/eos/pull/10414)) feature privacy enhancements
- ([#10419](https://github.com/EOSIO/eos/pull/10419)) be explict about config.hpp location; fixes CMP0115 warning on latest cmake
- ([#10427](https://github.com/EOSIO/eos/pull/10427)) feature privacy snapshot test
- ([#10428](https://github.com/EOSIO/eos/pull/10428)) fix homebrew sha256 warning
- ([#10450](https://github.com/EOSIO/eos/pull/10450)) fix bug in trace_api_plugin no params returned GH issue 10435 / EPE 1066 https://github.com/EOSIO/eos/issues/10435 #10449
- ([#10451](https://github.com/EOSIO/eos/pull/10451)) Add full cleos return object to final test assertion
- ([#10454](https://github.com/EOSIO/eos/pull/10454)) [develop] Update Anka nodes used for build script pipelines.
- ([#10457](https://github.com/EOSIO/eos/pull/10457)) Actually wait for the setup transaction to be added to a block.
- ([#10458](https://github.com/EOSIO/eos/pull/10458)) resolve flakiness in privacy scenario 3 test
- ([#10434](https://github.com/EOSIO/eos/pull/10434)) Always attempt to resync when receiving a block less than LIB.
- ([#10460](https://github.com/EOSIO/eos/pull/10460)) Support Stability Testing for Tests + CI Bug Fixes
- ([#10465](https://github.com/EOSIO/eos/pull/10465)) Bug Fixes for the eosio-test-stability Pipeline
- ([#10467](https://github.com/EOSIO/eos/pull/10467)) Another Bug Fix for the eosio-test-stability Pipeline
- ([#10470](https://github.com/EOSIO/eos/pull/10470)) Revert "Refactor block tracking to avoid re-sending to sender - develop"
- ([#10476](https://github.com/EOSIO/eos/pull/10476)) Add waits for token transfer block to improve reliability.
- ([#10471](https://github.com/EOSIO/eos/pull/10471)) Add validation to ensure transaction level resource_payer info matches transaction_extensions
- ([#10432](https://github.com/EOSIO/eos/pull/10432)) Update Zipkin connection retry - develop
- ([#10462](https://github.com/EOSIO/eos/pull/10462)) simplify build & fix binaries via a forked & modified libpqxx compatible with libpq 9.2
- ([#10481](https://github.com/EOSIO/eos/pull/10481)) multiversion test old version v2.0.9 -> release/2.1.x
- ([#10431](https://github.com/EOSIO/eos/pull/10431)) Test packaged EOSIO Linux artifacts against standard dockers.
- ([#10478](https://github.com/EOSIO/eos/pull/10478)) flakiness fix for eosio_blocklog_prune_test
- ([#10483](https://github.com/EOSIO/eos/pull/10483)) fix MacOS 10.15 pinned build problem with clang10
- ([#10485](https://github.com/EOSIO/eos/pull/10485)) Don't Always Push to DockerHub

## Documentation

- ([#9695](https://github.com/EOSIO/eos/pull/9695)) [docs] update docs to reflect updated code using 128MiB
- ([#9728](https://github.com/EOSIO/eos/pull/9728)) [docs] Update URL in net_api_plugin description
- ([#9729](https://github.com/EOSIO/eos/pull/9729)) [docs] Update some chain_api_plugin descriptions
- ([#9755](https://github.com/EOSIO/eos/pull/9755)) [docs] Remove sudo command from install/uninstall script instructions
- ([#9757](https://github.com/EOSIO/eos/pull/9757)) [docs] Add BlockVault client logger to nodeos reference
- ([#9737](https://github.com/EOSIO/eos/pull/9737)) Update how-to-replay-from-a-blocks.log.md
- ([#9765](https://github.com/EOSIO/eos/pull/9765)) added next_key_bytes
- ([#9774](https://github.com/EOSIO/eos/pull/9774)) [docs] Add new fields to producer_api_plugin create_snapshot endpoint
- ([#9797](https://github.com/EOSIO/eos/pull/9797)) [docs] Correct link to the chain plugin
- ([#9780](https://github.com/EOSIO/eos/pull/9780)) [docs] Fix schemata version and update nodeos plugins
- ([#9788](https://github.com/EOSIO/eos/pull/9788)) [docs] Corrections to nodeos storage and read modes
- ([#9807](https://github.com/EOSIO/eos/pull/9807)) [docs] update link to chain plugin to be relative
- ([#9817](https://github.com/EOSIO/eos/pull/9817)) [docs] Fix blockvault plugin explainer and C++ reference links
- ([#9873](https://github.com/EOSIO/eos/pull/9873)) chain api plugin / get_activated_protocol_features modifications
- ([#9907](https://github.com/EOSIO/eos/pull/9907)) Add MacOS 10.15 (Catalina) to list of supported OSs in README
- ([#9904](https://github.com/EOSIO/eos/pull/9904)) [docs] dev branch: add how to local testnet with consensus
- ([#9951](https://github.com/EOSIO/eos/pull/9951)) [docs] dev - improve annotation for db_update_i64
- ([#9946](https://github.com/EOSIO/eos/pull/9946)) [docs] cleos doc-a-thon feedback
- ([#9948](https://github.com/EOSIO/eos/pull/9948)) [docs] cleos doc-a-thon feedback-3
- ([#9947](https://github.com/EOSIO/eos/pull/9947)) [docs] cleos doc-a-thon feedback-2
- ([#9949](https://github.com/EOSIO/eos/pull/9949)) [docs] cleos doc-a-thon feedback-4
- ([#10007](https://github.com/EOSIO/eos/pull/10007)) [docs] Update various cleos how-tos and fix index
- ([#10022](https://github.com/EOSIO/eos/pull/10022)) [docs] develop: improve cleos how to
- ([#10025](https://github.com/EOSIO/eos/pull/10025)) Documentation - correct description for the return value of the kv_get_data intrinsic
- ([#10110](https://github.com/EOSIO/eos/pull/10110)) Fixed get_kv_table_rows documentation to match the current code behavior.
- ([#10183](https://github.com/EOSIO/eos/pull/10183)) [docs] Update Create key pairs How-to
- ([#10230](https://github.com/EOSIO/eos/pull/10230)) Add comment why include_delta of code_object just returns false
- ([#10241](https://github.com/EOSIO/eos/pull/10241)) huangminghuang/ship-max-retained-files
- ([#10291](https://github.com/EOSIO/eos/pull/10291)) [docs][Develop] Update to how to and command ref
- ([#10308](https://github.com/EOSIO/eos/pull/10308)) [doc][develop] Add resource_monitor_plugin documentation
- ([#10346](https://github.com/EOSIO/eos/pull/10346)) [docs][develop] - match new template for "how to link permissions" and "set action permission"
- ([#10362](https://github.com/EOSIO/eos/pull/10362)) [docs]command ref docs fix - fix broken link, replace Set, fix callout
- ([#10360](https://github.com/EOSIO/eos/pull/10360)) [docs] Update cleos net commands and net plugin description
- ([#10069](https://github.com/EOSIO/eos/pull/10069)) [docs] Update List Keypairs cleos How-to
- ([#10072](https://github.com/EOSIO/eos/pull/10072)) [docs] Update Import keys cleos How-to
- ([#10198](https://github.com/EOSIO/eos/pull/10198)) [docs] Update Query account info How-to - 2.0
- ([#10075](https://github.com/EOSIO/eos/pull/10075)) [docs] Update Query Transaction Info cleos How-to
- ([#10399](https://github.com/EOSIO/eos/pull/10399)) [docs] Delisted Ubuntu 16.04 from installation instructions
- ([#10178](https://github.com/EOSIO/eos/pull/10178)) [docs] Update Create an account How-to
- ([#10422](https://github.com/EOSIO/eos/pull/10422)) [docs] Add usage and split/extract/merge options to eosio-blocklog
- ([#10443](https://github.com/EOSIO/eos/pull/10443)) [docs] documentation for 2.2 - read-only queries - cleos updates
- ([#10395](https://github.com/EOSIO/eos/pull/10395)) [docs] add transaction sponsorship cleos how to
- ([#10492](https://github.com/EOSIO/eos/pull/10492)) [docs] Add privacy access feature documentation


## Thanks!
Special thanks to the community contributors that submitted patches for this release:
- **@conr2d**
- **@ndcgundlach**
- **@maoueh**
- **@georgipopivanov**
- **@killshot13**
