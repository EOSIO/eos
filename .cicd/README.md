# eosio
The [eosio](https://buildkite.com/EOSIO/eosio) and [eosio-build-unpinned](https://buildkite.com/EOSIO/eosio-build-unpinned) pipelines are the primary pipelines for the [eos](https://github.com/EOSIO/eos) repository, running with specific or default versions of our dependencies, respectively. Both run against every commit to a base branch or pull request, along with the [eosio-code-coverage](https://buildkite.com/EOSIO/eosio-code-coverage) pipeline.

The [eosio](https://buildkite.com/EOSIO/eosio) pipeline further triggers the [eosio-sync-from-genesis](https://buildkite.com/EOSIO/eosio-sync-from-genesis) and [eosio-resume-from-state](https://buildkite.com/EOSIO/eosio-resume-from-state) pipelines on each build, and the the [eosio-lrt](https://buildkite.com/EOSIO/eosio-lrt) pipeline on merge commits. Each of these pipelines are described in more detail below and in their respective READMEs.

<details>
<summary>See More</summary>

## Index
1. [Configuration](README.md#configuration)
   1. [Variables](README.md#variables)
   1. [Examples](README.md#examples)
1. [Pipelines](README.md#pipelines)
1. [See Also](README.md#see-also)

## Configuration
Most EOSIO pipelines are run any time you push a commit or tag to an open pull request in [eos](https://github.com/EOSIO/eos), any time you merge a pull request, and nightly. The [eosio-lrt](https://buildkite.com/EOSIO/eosio-lrt) pipeline only runs when you merge a pull request because it takes so long. Long-running tests are also run in the [eosio](https://buildkite.com/EOSIO/eosio) nightly builds, which have `RUN_ALL_TESTS='true'` set.

### Variables
Most pipelines in the organization have several environment variables that can be used to configure how the pipeline runs. These environment variables can be specified when manually triggering a build via the Buildkite UI.

Configure which platforms are run:
```bash
SKIP_LINUX='true|false'              # skip all steps on Linux distros
SKIP_MAC='true|false'                # skip all steps on Mac hardware
```
These will override more specific operating system declarations, and primarily exist to disable one of our two buildfleets should one be sick or the finite macOS agents are congested.

Configure which operating systems are built, tested, and packaged:
```bash
RUN_ALL_TESTS='true'                 # run all tests in the current build (including LRTs, overridden by SKIP* variables)
SKIP_AMAZON_LINUX_2='true|false'     # skip all steps for Amazon Linux 2
SKIP_CENTOS_7_7='true|false'         # skip all steps for Centos 7.7
SKIP_CENTOS_8='true|false'           # skip all steps for Centos 8
SKIP_MACOS_10_14='true|false'        # skip all steps for MacOS 10.14
SKIP_MACOS_10_15='true|false'        # skip all steps for MacOS 10.15
SKIP_MACOS_11='true|false'           # skip all steps for MacOS 11
SKIP_UBUNTU_16_04='true|false'       # skip all steps for Ubuntu 16.04
SKIP_UBUNTU_18_04='true|false'       # skip all steps for Ubuntu 18.04
SKIP_UBUNTU_20_04='true|false'       # skip all steps for Ubuntu 20.04
```

Configure which steps are executed for each operating system:
```bash
SKIP_BUILD='true|false'              # skip all build steps
SKIP_UNIT_TESTS='true|false'         # skip all unit tests
SKIP_WASM_SPEC_TESTS='true|false'    # skip all wasm spec tests
SKIP_SERIAL_TESTS='true|false'       # skip all integration tests
SKIP_LONG_RUNNING_TESTS='true|false' # skip all long running tests
SKIP_MULTIVERSION_TEST='true|false'  # skip all multiversion tests
SKIP_SYNC_TESTS='true|false'         # skip all sync tests
SKIP_PACKAGE_BUILDER='true|false'    # skip all packaging steps
```

Configure how the steps are executed:
```bash
PINNED='true|false'                  # use specific versions of dependencies instead of whatever version is provided by default on a given platform
TIMEOUT='##'                         # set timeout in minutes for all steps
```

### Examples
Build and test on Linux only:
```bash
SKIP_MAC='true'
```

Build and test on MacOS only:
```bash
SKIP_LINUX='true'
```

Skip all tests:
```bash
SKIP_UNIT_TESTS='true'
SKIP_WASM_SPEC_TESTS='true'
SKIP_SERIAL_TESTS='true'
SKIP_LONG_RUNNING_TESTS='true'
SKIP_MULTIVERSION_TEST='true'
SKIP_SYNC_TESTS='true'
```

## Pipelines
There are several eosio pipelines that exist and are triggered by pull requests, pipelines, or schedules:

Pipeline | Details
---|---
[eosio](https://buildkite.com/EOSIO/eosio) | [eos](https://github.com/EOSIO/eos) build, tests, and packaging with pinned dependencies; runs on every pull request and base branch commit, and nightly
[eosio-base-images](https://buildkite.com/EOSIO/eosio-base-images) | pack EOSIO dependencies into docker and Anka base-images nightly
[eosio-big-sur-beta](https://buildkite.com/EOSIO/eosio-big-sur-beta) | build and test [eos](https://github.com/EOSIO/eos) on macOS 11 "Big Sur" weekly
[eosio-build-scripts](https://buildkite.com/EOSIO/eosio-build-scripts) | run [eos](https://github.com/EOSIO/eos) build scripts nightly on empty operating systems
[eosio-build-unpinned](https://buildkite.com/EOSIO/eosio-build-unpinned) | [eos](https://github.com/EOSIO/eos) build and tests with platform-provided dependencies; runs on every pull request and base branch commit, and nightly
[eosio-code-coverage](https://buildkite.com/EOSIO/eosio-code-coverage) | assess [eos](https://github.com/EOSIO/eos) unit test coverage; runs on every pull request and base branch commit
[eosio-debug-build](https://buildkite.com/EOSIO/eosio-debug-build) | perform a debug build for [eos](https://github.com/EOSIO/eos) on every pull request and base branch commit
[eosio-lrt](https://buildkite.com/EOSIO/eosio-lrt) | runs tests that need more time on merge commits
[eosio-resume-from-state](https://buildkite.com/EOSIO/eosio-resume-from-state) | loads the current version of `nodeos` from state files generated by specific previous versions of `nodeos` in each [eosio](https://buildkite.com/EOSIO/eosio) build ([Documentation](https://github.com/EOSIO/auto-eks-sync-nodes/blob/master/pipelines/eosio-resume-from-state/README.md))
[eosio-sync-from-genesis](https://buildkite.com/EOSIO/eosio-sync-from-genesis) | sync the current version of `nodeos` past genesis from peers on common public chains as a smoke test, for each [eosio](https://buildkite.com/EOSIO/eosio) build
[eosio-test-stability](https://buildkite.com/EOSIO/eosio-test-stability) | prove or disprove test stability by running a test thousands of times

## See Also
- Buildkite
  - [DevDocs](https://github.com/EOSIO/devdocs/wiki/Buildkite)
  - [eosio-resume-from-state Documentation](https://github.com/EOSIO/auto-eks-sync-nodes/blob/master/pipelines/eosio-resume-from-state/README.md)
  - [Run Your First Build](https://buildkite.com/docs/tutorials/getting-started#run-your-first-build)
  - [Stability Testing](https://github.com/EOSIO/eos/blob/HEAD/.cicd/eosio-test-stability.md)
- [#help-automation](https://blockone.slack.com/archives/CMTAZ9L4D) Slack Channel

</details>
