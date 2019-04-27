# BATS Bash Testing

For each bash script we have, there should be a separate .bash file within ROOT/tests/bash-bats/.

- DRYRUN=true is required for all tests. This can be used to ensure the right commands are being run without executing them.
- **MacOSX: You must have bats installed: ([Source Install Instructions](https://github.com/bats-core/bats-core#installing-bats-from-source))** || `brew install bats-core`

 - Running all tests: 
    ```
    $ bats tests/bash-bats/*.bash
      ✓ [eosio_build_darwin] > Testing -y/NONINTERACTIVE/PROCEED
      ✓ [eosio_build_darwin] > Testing prompts
      ✓ [eosio_build_darwin] > Testing executions
      ✓ [helpers] > execute > dryrun
      ✓ [helpers] > execute > verbose
      ✓ [uninstall] > Usage is visible with right interaction
      ✓ [uninstall] > Testing user prompts
      ✓ [uninstall] > Testing executions
      ✓ [uninstall] > --force
      ✓ [uninstall] > --force + --full

      10 tests, 0 failures
    ```

---

### Running Docker Environments for Testing
  1. `docker run -v $HOME/BLOCKONE/eos:/eos -t ubuntu:16.04 bash -c "cd /eos && ./tests/bash-bats/bats-core/bin/bats tests/bash-bats/*.bash"`
      - You'll need to modify the volume path ($HOME/eos) to indicate where you've got eos cloned locally.
      - Use -t option for better output (required when using debug function to see output)
