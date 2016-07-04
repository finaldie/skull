[![Build Status](https://travis-ci.org/finaldie/skull.svg?branch=master)](https://travis-ci.org/finaldie/skull)
[![GitHub license](https://img.shields.io/github/license/finaldie/skull.svg)]()
[![Platform](https://img.shields.io/badge/platform-Linux-blue.svg)]()

Skull
=====
A fast to start, easy to maintain, high productivity serving framework<br>

- [Introduction](#Introduction)
 - [Module](#Module)
 - [Workflow](#Workflow)
 - [Service](#Service)
- [Releases](#Releases)
- [How to Build](#How to Build)
 - [Install Dependencies](#Install Dependencies)
 - [Build and Install](#Build and Install)
- [A Quick Demo](#A Quick Demo)
 - [Create a skull project](#Create a skull project)
 - [Play with skull](#Play with skull)
- [How to Create a Service](#How to Create a Service)
- [How to Check Metrics](#How to Check Metrics)
- [Existing Services](#Existing Services)

## Introduction
Skull provides the following features:
* Modular development
* Project management
* Processize
* Lockfree Environment
* Native Monitoring
* Native Async Network IO
* Native Background IO
* Native Timer
* Multi-language Support
* Service Shareable

It's based on [Google Protobuf][3] and [Flibs][4], target to _Linux_ platform. And _Skull_ is compose of 3 components: **skull-core**, **skull-user-api** and **skull-project-management-scripts**, besides of that logically there are also 3 major concepts in _Skull_: **Workflow**, **Module** and **Service**. Before using _Skull_, let's understand the core concepts first.

### Module
_Module_ is a independent logic set, it defines what kind of data/things we should use/do in this step.

### Workflow
_Workflow_ is more like a *transcation rules*, *oriented automator* or *pipeline*, it controls how the transcation works, execute the modules one by one until be finished. Multiple modules can be chosed to join in a workflow, and there also can be multiple workflows in _Skull_.
Each _Workflow_ has its own _SharedData_, every _Module_ belongs to this _Workflow_ can read/write it.

### Service
_Service_ is designed for managing the data, and provide a group of APIs to access the data. _Module_ can use these APIs to access/consume the data, then decide what you want to do. Also the _Service_ is shareable, it's highly recommended you to share your _Service_ to other *skull projects*, to make the world better.

## Releases
[Changelog](ChangeLog.md)

## How to Build
Use Ubuntu14.04 as an example.

### Install Dependencies
```console
apt-get install valgrind libyaml-dev python-yaml libprotobuf-dev protobuf-compiler libprotobuf-c0 libprotobuf-c0-dev protobuf-c-compiler expect

git clone git@github.com:finaldie/skull.git
cd skull
git submodule update --init --recursive
make dep
```

### Build and Install
```console
make -j4
make install
```

**Notes:**
 * `make install` may need `sudo` access
 * If you do not want to enable `jemalloc`, use `make -j4 disable_jemalloc=true` to build it

## A Quick Demo
After you installed skull into your system, you can run the following steps to
create your skull project.

### Create a skull project
[![skull demo 1](http://g.recordit.co/6yGrVG7i0s.gif)]()

**Notes:** By default, the new module with a example code which is used for echo-back message

### Play with skull
[![skull demo 2](http://g.recordit.co/vSON9N6nuV.gif)]()

## How to Create a Service
The following is the example of adding a service **s1**, and then add a api **get** for it.
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
After that, you can use service apis which defined in `skull/service.h` from a module to communicate a service :)

## Share your Service and Import from Others
Service is designed for sharing, each service is built for one single purpose or solve specific problem, share to others which help people to build the project easier than ever.


## How to Check Metrics
Currently, the [AdminModule][1] is a builtin module in skull, just connect to port `7759`, then you will see them.
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
 - status
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

[1]: https://github.com/finaldie/skull-admin-c
[2]: https://github.com/finaldie/skull-service-dns
[3]: https://developers.google.com/protocol-buffers/
[4]: https://github.com/finaldie/final_libs

# Contribution
Fork the repo, then apply your fixes/features via a PR. Any question, open an issue directly :)
