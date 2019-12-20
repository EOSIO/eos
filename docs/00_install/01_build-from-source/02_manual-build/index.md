---
content_title: EOSIO Manual Build
---

[[info | Manual Builds are for Advanced Developers]]
| These manual instructions are intended for advanced developers. The [Shell Scripts](../01_shell-scripts/index.md) should be the preferred method to build EOSIO from source. If the script fails or your platform is not supported, continue with the instructions below.

## EOSIO Dependencies

When performing a manual build, it is necessary to install specific software packages that the EOSIO software depends on. To learn more about these dependencies, visit the [EOSIO Software Dependencies](00_eosio-dependencies.md) section.

## Platforms

Shell commands are available to manually download, build, install, test, and uninstall the EOSIO software and dependencies for these [platforms](03_platforms/index.md).

## Out-of-source Builds

While building dependencies and EOSIO binaries, out-of-source builds are also supported. Refer to the `cmake` help for more information.

## Other Compilers

To override `clang`'s default compiler toolchain, add these flags to the `cmake` command within the above instructions:

`-DCMAKE_CXX_COMPILER=/path/to/c++ -DCMAKE_C_COMPILER=/path/to/cc`

## Debug Builds

For a debug build, add `-DCMAKE_BUILD_TYPE=Debug`. Other common build types include `Release` and `RelWithDebInfo`.
