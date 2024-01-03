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

The build instructions have been tested on Fedora Workstation 38 and recent Ubuntu.

## Executing program

The test programs can be found and executed from the build directory, e.g.

* buildDir/test/test_name_exe

## Version

v0.6.1 - stable release

## Authors

Copyright 2024 Michael Berry <trismegustis@gmail.com>

## License

This project is licensed under the MIT License - see the LICENSE file for details.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
