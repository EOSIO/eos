
[[info | Shell Scripts]]
| The build script is one of various automated shell scripts provided in the APIFINY repository for building, installing, and optionally uninstalling the APIFINY software and its dependencies. They are available in the `apifiny/scripts` folder.

The build script first installs all dependencies and then builds APIFINY. The script supports these [Operating Systems](../../index.md#supported-operating-systems). To run it, first change to the `~/apifiny/apifiny` folder, then launch the script:

```sh
$ cd ~/apifiny/apifiny
$ ./scripts/apifiny_build.sh
```

The build process writes temporary content to the `apifiny/build` folder. After building, the program binaries can be found at `apifiny/build/programs`.

[[info | What's Next?]]
| [Install APIFINY binaries](../03_install-apifiny-binaries.md)
