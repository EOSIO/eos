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
```
docker run --rm -ti -v $HOME/BLOCKONE/eos.bats:/eos ubuntu:16.04 bash -c "cd /eos && ./tests/bash-bats/bats-core/bin/bats -t tests/bash-bats/eosio_build_ubuntu.bash"
docker run --rm -ti -v $HOME/BLOCKONE/eos.bats:/eos ubuntu:18.04 bash -c "cd /eos && ./tests/bash-bats/bats-core/bin/bats -t tests/bash-bats/eosio_build_ubuntu.bash"
docker run --rm -ti -v $HOME/BLOCKONE/eos.bats:/eos amazonlinux:2 bash -c "cd /eos && ./tests/bash-bats/bats-core/bin/bats -t tests/bash-bats/eosio_build_amazonlinux.bash"
docker run --rm -ti -v $HOME/BLOCKONE/eos.bats:/eos centos:7 bash -c "cd /eos && ./tests/bash-bats/bats-core/bin/bats -t tests/bash-bats/eosio_build_centos.bash"

```


#### Start docker first, then run (keeping installed packages + faster tests)
```
docker run --name ubuntu16 -d -t -v $HOME/BLOCKONE/eos.bats:/eos ubuntu:16.04 /bin/bash
docker run --name ubuntu18 -d -t -v $HOME/BLOCKONE/eos.bats:/eos ubuntu:18.04 /bin/bash
docker run --name amazonlinux2 -d -t -v $HOME/BLOCKONE/eos.bats:/eos amazonlinux:2 /bin/bash
docker run --name centos7 -d -t -v $HOME/BLOCKONE/eos.bats:/eos centos:7 /bin/bash

echo "[Ubuntu 16]"
docker exec -it ubuntu16 bash -c "cd /eos && ./tests/bash-bats/bats-core/bin/bats -t tests/bash-bats/eosio_build_ubuntu.bash"
echo "[Ubuntu 18]"
docker exec -it ubuntu18 bash -c "cd /eos && ./tests/bash-bats/bats-core/bin/bats -t tests/bash-bats/eosio_build_ubuntu.bash"
echo "[AmazonLinux 2]"
docker exec -it amazonlinux2 bash -c "cd /eos && ./tests/bash-bats/bats-core/bin/bats -t tests/bash-bats/eosio_build_amazonlinux.bash"
echo "[Centos 7]"
docker exec -it centos7 bash -c "cd /eos && ./tests/bash-bats/bats-core/bin/bats -t tests/bash-bats/eosio_build_centos.bash"
echo "[Mac OSX]"
./tests/bash-bats/bats-core/bin/bats -t tests/bash-bats/eosio_build_darwin.bash 
```

- You'll need to modify the volume path ($HOME/BLOCKONE/eos.bats) to indicate where you've got eos cloned locally.
- Use -t option for better output (required when using debug function to see output)
