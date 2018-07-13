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
Use _Ubuntu14.04_ as an example. (Tested on `Debian jessie/stretch`, `Ubuntu 12.04/14.04/16.04/18.04, alpine`, `RHEL6.x` and `Raspberry OS`)

### Install Dependencies
```console
# Install System Dependencies
sudo apt-get install autoconf valgrind expect libyaml-dev python3-dev python3-pip libprotobuf-c0-dev protobuf-c-compiler;
sudo pip install PyYAML pympler WebOb;

# Clone and Build Dependencies (For example: project folder is 'skull')
cd skull
git submodule update --init --recursive
make -j4 dep
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

**More Options:**
 * `python_path`: By default it's python3, but we can override it to another path for testing purpose

### Docker Images
Also, the [**_Docker images_**][31] are ready now, if people don't want to waste time to set up a brand new environment, we can run the **_Docker_** image directly within 1 min :)

Assume that we've already [installed _Docker_][30], then apply the following commands to pull and run the image to bring user into a development ready environment super fast:
```console
-bash$ docker pull finaldie/skull:1.1-build
-bash$ docker run -it finaldie/skull:1.1-build
```

And the below table is the current images:<br>

Tag              | Dockerfile         | Notes                         |
-----------------|--------------------|-------------------------------|
1.1              | ([Dockerfile][20]) | 1.1 Debian runtime image      |
1.1-build        | ([Dockerfile][21]) | 1.1 Debian dev/building image |
1.1-ubuntu       | ([Dockerfile][22]) | 1.1 Ubuntu runtime image      |
1.1-ubuntu-build | ([Dockerfile][23]) | 1.1 Ubuntu dev/building image |
1.1-alpine       | ([Dockerfile][24]) | 1.1 Alpine runtime image      |
1.1-alpine-build | ([Dockerfile][25]) | 1.1 Alpine dev/building image |

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
 - Discuss at Reddit: [https://www.reddit.com/r/skullengine/][18]
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
[18]: https://www.reddit.com/r/skullengine/


[20]: https://github.com/finaldie/dockerfiles/blob/master/skull/1.1/Dockerfile
[21]: https://github.com/finaldie/dockerfiles/blob/master/skull/1.1/Dockerfile.build
[22]: https://github.com/finaldie/dockerfiles/blob/master/skull/1.1/ubuntu/Dockerfile
[23]: https://github.com/finaldie/dockerfiles/blob/master/skull/1.1/ubuntu/Dockerfile.build
[24]: https://github.com/finaldie/dockerfiles/blob/master/skull/1.1/alpine/Dockerfile
[25]: https://github.com/finaldie/dockerfiles/blob/master/skull/1.1/alpine/Dockerfile.build

[30]: https://docs.docker.com/engine/installation/linux/docker-ce/ubuntu/
[31]: https://hub.docker.com/r/finaldie/skull/
