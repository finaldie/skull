[![Build Status](https://travis-ci.org/finaldie/skull.svg?branch=master)](https://travis-ci.org/finaldie/skull)
[![GitHub license](https://img.shields.io/github/license/finaldie/skull.svg)]()
[![Platform](https://img.shields.io/badge/platform-Linux-blue.svg)]()

Skull
=====
_Skull_ is a event-driven serving framework with multiple modern designs to allow user:
* Fast to create a prototype
* Easy to maintain even a huge project
* Write code in lock-free environment

It helps people to build an application/server extremely fast and strong, high scalability and flexibility in application layer, and with strong performance in engine level. Read more [here][6].

_Skull_ can be used in generic serving layer or embedded device. E.g. _web logic server_, _game server_, etc.

## Releases
[Changelog](ChangeLog.md)

## How to Build
Use _Ubuntu14.04_ as an example. (Tested on `Ubuntu 12.04/14.04/16.04`, `RHEL6.x` and `Raspberry OS`)

### Install Dependencies
```console
# Install System Dependencies
sudo apt-get install autoconf valgrind expect libyaml-dev python-dev python-pip libprotobuf-dev protobuf-compiler libprotobuf-c0-dev protobuf-c-compiler;
sudo pip install PyYAML protobuf==2.6.1 pympler WebOb;

# Clone and Build Dependencies (For example: project folder is 'skull')
cd skull
git submodule update --init --recursive
make dep
sudo make install-dep
```

### Build and Install
```console
make -j4
sudo make install
```

**Notes:**
 * To disable `jemalloc`, use `make -j4 disable_jemalloc=true` to build it
 * For some _Linux_ Releases, we might need to use `CFLAGS`, `CXXFLAGS`, `LDFLAGS` to finish the build

## A Quick Demo
After installing _Skull_ into the system, run the following steps to
create a _Skull_ project, have fun :)

### Create a Skull Project
[![skull demo 1](http://g.recordit.co/6yGrVG7i0s.gif)]()

**Notes:**
 * By default, a new module contains the example code which is used to echo-back message
 * Above creation is for `C++` module, type `py` in language selection step if needed

### Playing with Skull
[![skull demo 2](http://g.recordit.co/vSON9N6nuV.gif)]()

## Existing Services
Name                  | Description |
----------------------|-------------|
[DNS Client][2] | Async DNS client for A record |
[Http Client][15] | Async http client service, easy to send/receive http request/response |

## Other Resources
Name                  | Description |
----------------------|-------------|
[Skull-Perf Cases][5] | Including some basic perf cases |
[DNSTurbo][16]        | Smart DNS Client based on _Skull_. [Trailer][17] |

## Documentations
* [High Level Introduction][6]
* [How To Start][7]
* [How to Test][8]
* [How to Deploy][9]
* [Monitoring][10]
* [API Docs - Cpp][11]
* [API Docs - Python][12]
* [Integrate with Nginx][13]

## Contribution and Discussion
To discuss any issues, there are some ways we can use:
 - Open an issue on Github directly
 - Send an email to skull-engine@googlegroups.com
 - Go directly to the [Mail Group][14], and post the questions.

To fix a bug or add a new feature, just **`Fork`** the repo, then apply the fixes/features via a PR.

[1]: https://github.com/finaldie/skull-admin-c
[2]: https://github.com/finaldie/skull-service-dns
[3]: https://developers.google.com/protocol-buffers/
[4]: https://github.com/finaldie/final_libs
[5]: https://github.com/finaldie/skull-perf
[6]: https://github.com/finaldie/skull/wiki
[7]: https://github.com/finaldie/skull/wiki/How-To-Start
[8]: https://github.com/finaldie/skull/wiki/How-To-Test
[9]: https://github.com/finaldie/skull/wiki/How-To-Deploy
[10]: https://github.com/finaldie/skull/wiki/Monitoring
[11]: https://github.com/finaldie/skull/wiki/API-Doc-:-Cpp
[12]: https://github.com/finaldie/skull/wiki/API-Doc-:-Python
[13]: https://github.com/finaldie/skull/wiki/Integrate-with-Nginx
[14]: https://groups.google.com/forum/#!forum/skull-engine
[15]: https://github.com/finaldie/skull-service-httpcli
[16]: https://github.com/finaldie/DNSTurbo
[17]: https://github.com/finaldie/DNSTurbo#trailer

