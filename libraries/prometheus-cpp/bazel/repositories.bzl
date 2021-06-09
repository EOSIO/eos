load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

def prometheus_cpp_repositories():
    maybe(
        http_archive,
        name = "civetweb",
        strip_prefix = "civetweb-1.14",
        sha256 = "d02d7ab091c8b4edf21fc13a03c6db08a8a8b8605e35e0073251b9d88443c653",
        urls = [
            "https://github.com/civetweb/civetweb/archive/v1.14.tar.gz",
        ],
        build_file = "@com_github_jupp0r_prometheus_cpp//bazel:civetweb.BUILD",
    )

    maybe(
        http_archive,
        name = "com_google_googletest",
        sha256 = "9dc9157a9a1551ec7a7e43daea9a694a0bb5fb8bec81235d8a1e6ef64c716dcb",
        strip_prefix = "googletest-release-1.10.0",
        urls = [
            "https://github.com/google/googletest/archive/release-1.10.0.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "com_github_curl",
        sha256 = "5f85c4d891ccb14d6c3c701da3010c91c6570c3419391d485d95235253d837d7",
        strip_prefix = "curl-7.76.1",
        urls = [
            "https://github.com/curl/curl/releases/download/curl-7_76_1/curl-7.76.1.tar.gz",
            "https://curl.haxx.se/download/curl-7.76.1.tar.gz",
        ],
        build_file = "@com_github_jupp0r_prometheus_cpp//bazel:curl.BUILD",
    )

    maybe(
        http_archive,
        name = "com_github_google_benchmark",
        sha256 = "e4fbb85eec69e6668ad397ec71a3a3ab165903abe98a8327db920b94508f720e",
        strip_prefix = "benchmark-1.5.3",
        urls = [
            "https://github.com/google/benchmark/archive/v1.5.3.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "net_zlib_zlib",
        sha256 = "c3e5e9fdd5004dcb542feda5ee4f0ff0744628baf8ed2dd5d66f8ca1197cb1a1",
        strip_prefix = "zlib-1.2.11",
        urls = [
            "https://mirror.bazel.build/zlib.net/zlib-1.2.11.tar.gz",
            "https://zlib.net/zlib-1.2.11.tar.gz",
        ],
        build_file = "@com_github_jupp0r_prometheus_cpp//bazel:zlib.BUILD",
    )
