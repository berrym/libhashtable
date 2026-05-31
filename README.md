# libhashtable

## Description

A small generic hashtable library written in C with type wrapped implementations for common table types.

## Getting Started

Install meson and ninja build system

Clone the git repository:

* git clone https://github.com/berrym/libhashtable.git

Build:

For modern meson users:

* meson setup buildDir
* cd buildDir
* meson compile

For legacy meson users:

* meson setup buildDir
* cd buildDir
* meson ninja


## Executing program

The test programs can be found and executed from the build directory, e.g.

* buildDir/test/test_name_exe

## Hash width

The FNV-1a hash returns either a 32-bit or a 64-bit value. The width is
selected at build time:

* default (`-Dhash_width=auto`): the header picks 32 or 64 from `UINTPTR_MAX`,
  so the typed wrappers and FNV-1a constants agree with the host word size.
* `-Dhash_width=32` or `-Dhash_width=64`: force a specific width, useful for
  hash-stability tests or cross-compile vectors.

The hash width is also overridable directly from the compiler with
`-DHT_HASH_WIDTH=32` or `-DHT_HASH_WIDTH=64`. Legacy `-DCPU_32_BIT` and
`-DCPU_64_BIT` defines are still recognized when `HT_HASH_WIDTH` is not set.

The previously separate `32bit` and `64bit` branches are deprecated as of
v0.7.0. Their final commits are preserved at the tags `archive/32bit-final`
and `archive/64bit-final`. All work now happens on `master`.

## Version

v0.7.0 - stable release

## Authors

Copyright 2024 Michael Berry <trismegustis@gmail.com>

## License

This project is licensed under the MIT License - see the LICENSE file for details.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![build result](https://build.opensuse.org/projects/home:berrym/packages/libhashtable-devel/badge.svg?type=default)](https://build.opensuse.org/package/show/home:berrym/libhashtable-devel)
[![Copr build status](https://copr.fedorainfracloud.org/coprs/mberry/libhashtable-devel/package/libhashtable-devel/status_image/last_build.png)](https://copr.fedorainfracloud.org/coprs/mberry/libhashtable-devel/package/libhashtable-devel/)
