# BATS Bash Testing

For each bash script we have, there should be a separate .bash file within REPO_ROOT/tests/bash-bats/.

- DRYRUN=true is required for all tests and automatically enabled. You can use this when you're manually running eosio_build.bash (`DRYRUN=true VERBOSE=true ./scripts/eosio_build.bash`)

 - Running all tests: 
    ```
    $ ./tests/bash-bats/bats-core/bin/bats tests/bash-bats/*.bash
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
echo "[Darwin]"
./tests/bash-bats/bats-core/bin/bats -t tests/bash-bats/*.bash
echo "[Ubuntu 16]"
docker run --rm -ti -v $HOME/BLOCKONE/eos.bats:/eos ubuntu:16.04 bash -c "cd /eos && ./tests/bash-bats/bats-core/bin/bats -t tests/bash-bats/*.bash"
echo "[Ubuntu 18]"
docker run --rm -ti -v $HOME/BLOCKONE/eos.bats:/eos ubuntu:18.04 bash -c "cd /eos && ./tests/bash-bats/bats-core/bin/bats -t tests/bash-bats/*.bash"
echo "[AmazonLinux 2]"
docker run --rm -ti -v $HOME/BLOCKONE/eos.bats:/eos amazonlinux:2 bash -c "cd /eos && ./tests/bash-bats/bats-core/bin/bats -t tests/bash-bats/*.bash"
echo "[Centos 7]"
docker run --rm -ti -v $HOME/BLOCKONE/eos.bats:/eos centos:7 bash -c "cd /eos && ./tests/bash-bats/bats-core/bin/bats -t tests/bash-bats/*.bash"

```

#### Start docker first, then run (keeping installed packages + faster tests)
```
docker run --name ubuntu16 -d -t -v $HOME/BLOCKONE/eos.bats:/eos ubuntu:16.04 /bin/bash
docker run --name ubuntu18 -d -t -v $HOME/BLOCKONE/eos.bats:/eos ubuntu:18.04 /bin/bash
docker run --name amazonlinux2 -d -t -v $HOME/BLOCKONE/eos.bats:/eos amazonlinux:2 /bin/bash
docker run --name centos7 -d -t -v $HOME/BLOCKONE/eos.bats:/eos centos:7 /bin/bash

echo "[Darwin]"
./tests/bash-bats/bats-core/bin/bats -t tests/bash-bats/*.bash 
echo "[Ubuntu 16]"
docker exec -it ubuntu16 bash -c "cd /eos && ./tests/bash-bats/bats-core/bin/bats -t tests/bash-bats/*.bash"
echo "[Ubuntu 18]"
docker exec -it ubuntu18 bash -c "cd /eos && ./tests/bash-bats/bats-core/bin/bats -t tests/bash-bats/*.bash"
echo "[AmazonLinux 2]"
docker exec -it amazonlinux2 bash -c "cd /eos && ./tests/bash-bats/bats-core/bin/bats -t tests/bash-bats/*.bash"
echo "[Centos 7]"
docker exec -it centos7 bash -c "cd /eos && ./tests/bash-bats/bats-core/bin/bats -t tests/bash-bats/*.bash"
```

- You'll need to modify the volume path ($HOME/BLOCKONE/eos.bats) to indicate where you've got eos cloned locally.
- Use -t option for better output (required when using debug function to see output)
