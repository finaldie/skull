[![Build Status](https://travis-ci.org/finaldie/skull.svg?branch=master)](https://travis-ci.org/finaldie/skull)
[![GitHub license](https://img.shields.io/github/license/finaldie/skull.svg)]()
[![Platform](https://img.shields.io/badge/platform-Linux-blue.svg)]()

Skull
=====
A fast to start, easy to maintain, high productive serving framework<br>

- [Introduction](#introduction)
 - [Module](#module)
 - [Workflow](#workflow)
 - [Service](#service)
- [Releases](#releases)
- [How to Build](#how-to-build)
 - [Install Dependencies](#install-dependencies)
 - [Build and Install](#build-and-install)
- [A Quick Demo](#a-quick-demo)
 - [Create a Skull Project](#create-a-skull-project)
 - [Play with skull](#play-with-skull)
- [How to Create a Service](#how-to-create-a-service)
- [How to Check Counters](#how-to-check-counters)
- [Existing Services](#existing-services)
- [Other Resources](#other-resources)
- [Contribution](#contribution)

## Introduction
Skull provides the following key features:
* Modular Development Environment
* Project Management
* Processize
* Lockfree Environment
* Native Monitoring
* Native Async Network IO
* Native Background IO
* Native Timer
* Multi-Language Support (**_C/C++_**, **_Python_**)
* Integrated with _Nginx_
* Service Shareable

It's based on [Google Protobuf][3] and [Flibs][4], target to _Linux_ platform. _Skull_ is consist of 3 components: **skull-core**, **skull-user-api** and **skull-project-management-scripts**, and there are 3 major concepts in _Skull_: **Workflow**, **Module** and **Service**. Before using it, let's understand the core concepts first.

### Module
_Module_ is a independent logic set, it defines what kind of data/things we should use/do in this step.

### Workflow
_Workflow_ is more like a *transaction rules*, *oriented automator* or *pipeline*, it controls how the transaction works, execute the modules one by one until be finished. Multiple modules can be chosen to join in a workflow, and there also can be multiple workflows in _Skull_.
Each _Workflow_ has its own _SharedData_, every _Module_ belongs to this _Workflow_ can read/write it.

### Service
_Service_ is designed for managing the data, and provide a group of APIs to access the data. _Module_ can use these APIs to access/consume the data, then decide what you want to do. Also the _Service_ is shareable, it's highly recommended that user to share their _Service_ to other *skull projects*, to make the world better.

## Releases
[Changelog](ChangeLog.md)

## Documentations
* [High Level Introduction][6]
* [How To Start][7]
* [API Docs - Cpp][8]
* [API Docs - Python][9]
* [Integrate with Nginx][10]

## How to Build
Use Ubuntu14.04 as an example.

### Install Dependencies
```console
sudo apt-get install valgrind expect libyaml-dev python-dev python-pip libprotobuf-dev protobuf-compiler libprotobuf-c0-dev protobuf-c-compiler
sudo pip install PyYAML protobuf pympler WebOb

git clone git@github.com:finaldie/skull.git
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
 * For some _Linux_ Releases, user might need to use `CFLAGS`, `CXXFLAGS`, `LDFLAGS` to finish the build

## A Quick Demo
After installing _Skull_ into the system, run the following steps to
create a _Skull_ project.

### Create a Skull Project
[![skull demo 1](http://g.recordit.co/6yGrVG7i0s.gif)]()

**Notes:**
 * By default, a new module contains the example code which is used to echo-back message
 * Above creation is for C++ module, type `py` in language selection step if needed

### Play with Skull
[![skull demo 2](http://g.recordit.co/vSON9N6nuV.gif)]()

## How to Create a Service
The following is an example of adding a service `s1`, and then add an API `get` to it.
```console
bash $> skull service -add
service name? s1
which language the service belongs to? (cpp) cpp
data mode? (rw-pr | rw-pw) rw-pr
notice: the common/cpp folder has already exist, ignore it
service [s1] added successfully
bash $> cd src/services/s1
bash $> skull service --api-add
service api name: get
s1-get_req added
s1-get_resp added
service api get added successfully
```
After that, use service APIs defined in `skullcpp/service.h` from a module to communicate with the service :)

## Share your Service and Import from Others
Service is designed for sharing, each service is built for one single purpose or solving a specific problem, share to others, help people to build their project easier than ever.

## How to Check Counters
Currently, the [AdminModule][1] is a builtin module in _Skull_, just connect to port `7759`:
```console
final@ubuntu: ~>telnet 0 7759
Trying 0.0.0.0...
Connected to 0.
Escape character is '^]'.
Trying 0.0.0.0...
Connected to 0.
Escape character is '^]'.
help
commands:
 - help
 - metrics
 - last
 - status|info
metrics
2015:12:26_01:36:54 to 2015:12:26_01:37:33
skull.core.g.global.timer_complete: 39.000000
skull.core.g.global.connection_create: 1.000000
skull.core.g.global.response: 1.000000
skull.core.g.global.entity_create: 43.000000
skull.core.g.global.uptime: 39.000000
skull.core.g.global.entity_destroy: 39.000000
skull.core.g.global.latency: 148.000000
skull.core.g.global.timer_emit: 41.000000
skull.core.g.global.request: 2.000000
skull.core.t.worker.master.timer_complete: 39.000000
skull.core.t.worker.master.entity_create: 43.000000
skull.core.t.worker.master.entity_destroy: 39.000000
skull.core.t.worker.master.timer_emit: 41.000000
skull.core.t.worker.worker-0.accept: 1.000000
skull.core.t.worker.worker-0.latency: 148.000000
skull.core.t.worker.worker-0.response: 1.000000
skull.core.t.worker.worker-0.request: 2.000000
```

# Existing Services
Name                  | Description |
----------------------|-------------|
[Async DNS Client][2] | Example service, to show how to write a basic async DNS client |

# Other Resources
Name                  | Description |
----------------------|-------------|
[Skull-Perf Cases][5] | Including some basic perf cases |



[1]: https://github.com/finaldie/skull-admin-c
[2]: https://github.com/finaldie/skull-service-dns
[3]: https://developers.google.com/protocol-buffers/
[4]: https://github.com/finaldie/final_libs
[5]: https://github.com/finaldie/skull-perf
[6]: https://github.com/finaldie/skull/wiki
[7]: https://github.com/finaldie/skull/wiki/How-To-Start
[8]: https://github.com/finaldie/skull/wiki/API-Doc-:-Cpp
[9]: https://github.com/finaldie/skull/wiki/API-Doc-:-Python
[10]: https://github.com/finaldie/skull/wiki/Integrate-with-Nginx

# Contribution
Fork the repo, then apply your fixes/features via a PR. Important things before sending a PR, be sure you have added/fixed a(n) new/existing test case, and passed it :)
