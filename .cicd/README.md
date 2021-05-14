# eosio
The [eosio](https://buildkite.com/EOSIO/eosio) pipeline is the primary CI/CD pipeline for members of the EOSIO organization developing on the EOSIO/eos repository. The [eosio-build-unpinned](https://buildkite.com/EOSIO/eosio-build-unpinned) pipeline is also executed regularly. The difference in these two pipelines is whether the compiler and other dependencies are pinned to specific versions. The eosio pipeline uses pinned compilers/dependencies while the eosio-build-unpinned pipeline avoids pinning dependencies as much as possible.

<details>
<summary>More Info</summary>

## Index
1. [Variables](https://github.com/EOSIO/eos/blob/release/2.1.x/.cicd/README.md#variables)
1. [Examples](https://github.com/EOSIO/eos/blob/release/2.1.x/.cicd/README.md#examples)
1. [Pipelines](https://github.com/EOSIO/eos/blob/release/2.1.x/.cicd/README.md#pipelines)
1. [See Also](https://github.com/EOSIO/eos/blob/release/2.1.x/.cicd/README.md#see-also)

### Variables
Most pipelines in the organization have several environment variables that can be used to configure how the pipeline runs. These environment variables can be specified when manually triggering a build via the Buildkite UI.

Configure which operating systems are built, tested, and packaged:
```bash
SKIP_LINUX='true|false'              # true skips all build/test/packaging steps on Linux distros
SKIP_MAC='true|false'                # true skips all build/test/packaging steps on Mac hardware
SKIP_AMAZON_LINUX_2='true|false'     # true skips all build/test/packaging steps for Amazon Linux 2
SKIP_CENTOS_7_7='true|false'         # true skips all build/test/packaging steps for Centos 7
SKIP_CENTOS_8='true|false'           # true skips all build/test/packaging steps for Centos 8
SKIP_MACOS_10_14='true|false'        # true skips all build/test/packaging steps for MacOS 10.14
SKIP_MACOS_10_15='true|false'        # true skips all build/test/packaging steps for MacOS 10.15
SKIP_MACOS_11='true|false'           # true skips all build/test/packaging steps for MacOS 11
SKIP_UBUNTU_16_04='true|false'       # true skips all build/test/packaging steps for Ubuntu 16.04
SKIP_UBUNTU_18_04='true|false'       # true skips all build/test/packaging steps for Ubuntu 18.04
SKIP_UBUNTU_20_04='true|false'       # true skips all build/test/packaging steps for Ubuntu 20.04
```

Configure which steps are executed for each operating system:
```bash
SKIP_BUILD='true|false'              # true skips all build steps for all distros
SKIP_UNIT_TESTS='true|false'         # true skips all unit test executions for all distros
SKIP_WASM_SPEC_TESTS='true|false'    # true skips all wasm spec test executions for all distros
SKIP_SERIAL_TESTS='true|false'       # true skips all integration test executions for all distros
SKIP_LONG_RUNNING_TESTS='true|false' # true skips all long running test executions for all distros
SKIP_MULTIVERSION_TEST='true|false'  # true skips all multiversion tests
SKIP_SYNC_TESTS='true|false'         # true skips all sync tests
SKIP_PACKAGE_BUILDER='true|false'    # true skips all package building steps for all distros
```

Configure how the steps are executed:
```bash
PINNED='true|false'                  # controls compiler/dependency pinning
TIMEOUT='##'                         # controls timeout in minutes for all steps
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
There are several eosio pipelines that are exist and executed via pull requests, triggered from other builds, or scheduled to run on a regular basis:


Pipeline | Details
---|---
[eosio](https://buildkite.com/EOSIO/eosio) | Primary pipeline for the EOSIO/eos Github repo. It is triggered when a pull request is created.
[eosio-build-unpinned](https://buildkite.com/EOSIO/eosio-build-unpinned) | Pipeline that performs a build without a pinned compiler. It is triggered when a pull request is created.
[eosio-lrt](https://buildkite.com/EOSIO/eosio-lrt) | Pipeline that only executes the long running tests. It is triggered after a pull request is merged.
[eosio-base-images](https://buildkite.com/EOSIO/eosio-base-images) | Pipeline that ensures all MacOS VM and Docker container builders can be built. It is scheduled for periodic execution.
[eosio-build-scripts](https://buildkite.com/EOSIO/eosio-build-scripts) | Pipeline that ensure the build scripts function. It is scheduled for periodic execution.
[eosio-big-sur-beta](https://buildkite.com/EOSIO/eosio-big-sur-beta) | Pipeline that performs a build only using MacOS 11 builders. It is scheduled for periodic execution.
[eosio-sync-from-genesis](https://buildkite.com/EOSIO/eosio-sync-from-genesis) | Pipeline that ensures built code can sync properly. It is triggered during pull request builds.
[eosio-resume-from-state](https://buildkite.com/EOSIO/eosio-resume-from-state) | Pipeline that ensures that built binaries can resume from previous binary versions. It is triggered during pull request builds.

</details>

## See Also
- [Buildkite Documentation](https://github.com/EOSIO/devdocs/wiki/Buildkite) (internal)
- [EOSIO Resume from State Documentation](https://github.com/EOSIO/auto-eks-sync-nodes/blob/master/pipelines/eosio-resume-from-state/README.md) (internal)
- [#help-automation](https://blockone.slack.com/archives/CMTAZ9L4D) (internal)
