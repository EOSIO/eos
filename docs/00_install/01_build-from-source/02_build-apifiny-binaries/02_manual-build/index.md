# APIFINY Manual Build

[[info | Manual Builds are for Advanced Developers]]
| These manual instructions are intended for advanced developers. The [Build Script](../00_build-script.md) should be the preferred method to build APIFINY from source. If the script fails or your platform is not supported, continue with the instructions below.

## APIFINY Dependencies

When performing a manual build, it is necessary to install specific software packages that the APIFINY software depends on. To learn more about these dependencies, visit the [APIFINY Software Dependencies](00_apifiny-dependencies/index.md) section.

## Instructions

The following instructions will build the APIFINY dependencies and APIFINY binaries manually by invoking commands on the shell:

1. [Manual Install APIFINY Dependencies](00_apifiny-dependencies/index.md#manual-installation-of-dependencies)
2. [Manual Build APIFINY Binaries](01_apifiny-manual-build.md)

## Out-of-source Builds

While building dependencies and APIFINY binaries, out-of-source builds are also supported. Refer to the `cmake` help for more information.

## Other Compilers

To override `clang`'s default compiler toolchain, add these flags to the `cmake` command within the above instructions:

`-DCMAKE_CXX_COMPILER=/path/to/c++ -DCMAKE_C_COMPILER=/path/to/cc`

## Debug Builds

For a debug build, add `-DCMAKE_BUILD_TYPE=Debug`. Other common build types include `Release` and `RelWithDebInfo`.
