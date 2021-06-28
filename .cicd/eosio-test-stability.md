# Stability Testing
Stability testing of EOSIO unit and integration tests is done in the [eosio-test-stability](https://buildkite.com/EOSIO/eosio-test-stability) pipeline. It will take thousands of runs of any given test to identify it as "stable" or "unstable". Runs should be split evenly across "pinned" (fixed dependency version) and "unpinned" (default dependency version) builds because, sometimes, test instability is only expressed in one of these environments. Finally, stability testing should be performed on the Linux fleet first because this fleet is effectively infinite. Once stability is demonstrated on Linux, testing can be performed on the finite macOS Anka fleet.

<details>
<summary>See More</summary>

## Index
1. [Configuration](eosio-test-stability.md#configuration)
   1. [Variables](eosio-test-stability.md#variables)
   1. [Runs](eosio-test-stability.md#runs)
   1. [Examples](eosio-test-stability.md#examples)
1. [See Also](eosio-test-stability.md#see-also)

## Configuration
The [eosio-test-stability](https://buildkite.com/EOSIO/eosio-test-stability) pipeline uses the same pipeline upload script as [eosio](https://buildkite.com/EOSIO/eosio), [eosio-build-unpinned](https://buildkite.com/EOSIO/eosio-build-unpinned), and [eosio-lrt](https://buildkite.com/EOSIO/eosio-lrt), so all variables from the [pipeline documentation](README.md) apply.

### Variables
There are five primary environment variables relevant to stability testing:
```bash
PINNED='true|false'    # whether to perform the test with pinned dependencies, or default dependencies
ROUNDS='ℕ'             # natural number defining the number of gated rounds of tests to generate
ROUND_SIZE='ℕ'         # number of test steps to generate per operating system, per round
SKIP_MAC='true|false'  # conserve finite macOS Anka agents by excluding them from your testing
TEST='name'            # PCRE expression defining the tests to run, preceded by '^' and followed by '$'
TIMEOUT='ℕ'            # set timeout in minutes for all Buildkite steps
```
The `TEST` variable is parsed as [pearl-compatible regular expression](https://www.debuggex.com/cheatsheet/regex/pcre) where the expression in `TEST` is preceded by `^` and followed by `$`. To specify one test, set `TEST` equal to the test name (e.g. `TEST='read_only_query'`). Specify two tests as `TEST='(nodeos_short_fork_take_over_lr_test|read_only_query)'`. Or, perhaps, you want all of the `restart_scenarios` tests. Then, you could define `TEST='restart-scenario-test-.*'` and Buildkite will generate `ROUND_SIZE` steps each round for each operating system for all three restart scenarios tests.

### Runs
The number of total test runs will be:
```bash
RUNS = ROUNDS * ROUND_SIZE * OS_COUNT * TEST_COUNT # where:
OS_COUNT   = 'ℕ' # the number of supported operating systems
TEST_COUNT = 'ℕ' # the number of tests matching the PCRE filter in TEST
```

### Examples
We recommend stability testing one test per build with two builds per test, on Linux at first. Kick off one pinned build on Linux...
```bash
PINNED='true'
ROUNDS='42'
ROUND_SIZE'5'
SKIP_MAC='true'
TEST='read_only_query'
```
...and one unpinned build on Linux:
```bash
PINNED='true'
ROUNDS='42'
ROUND_SIZE'5'
SKIP_MAC='true'
TEST='read_only_query'
```
Once the Linux runs have proven stable, and if instability was observed on macOS, kick off two equivalent builds on macOS instead of Linux. One pinned build on macOS...
```bash
PINNED='true'
ROUNDS='42'
ROUND_SIZE'5'
SKIP_LINUX='true'
SKIP_MAC='false'
TEST='read_only_query'
```
...and one unpinned build on macOS:
```bash
PINNED='true'
ROUNDS='42'
ROUND_SIZE'5'
SKIP_LINUX='true'
SKIP_MAC='false'
TEST='read_only_query'
```
If these runs are against `eos:develop` and `develop` has five supported operating systems, this pattern would consist of 2,100 runs per test across all four builds. If the runs are against `eos:release/2.1.x` which, at the time of this writing, supports eight operating systems, this pattern would consist of 3,360 runs per test across all four builds. This gives you and your team strong confidence that any test instability occurs less than 1% of the time.

# See Also
- Buildkite
  - [DevDocs](https://github.com/EOSIO/devdocs/wiki/Buildkite)
  - [EOSIO Pipelines](https://github.com/EOSIO/eos/blob/HEAD/.cicd/README.md)
  - [Run Your First Build](https://buildkite.com/docs/tutorials/getting-started#run-your-first-build)
- [#help-automation](https://blockone.slack.com/archives/CMTAZ9L4D) Slack Channel

</details>
