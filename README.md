[![Build Status](https://travis-ci.org/finaldie/skull.svg?branch=master)](https://travis-ci.org/finaldie/skull)

skull
=====

Fast to start-up, easy to maintain, high productivity serving framework.

## Releases
[Changelog](ChangeLog.md)

## How to Build
Use Ubuntu14.04 as an example.

### Install dependencies
```console
apt-get install valgrind libyaml-dev python-yaml libprotobuf-c0 libprotobuf-c0-dev protobuf-c-compiler libprotobuf-dev

git clone git@github.com:finaldie/skull.git
cd skull
git submodule update --init --recursive
make dep
```

### Build
```console
make -j4
```

### Install Skull and its related Scripts
```console
make install
```

## A Quick Demo
After you installed skull into your system, you can run the following steps to
create your skull project.

### Create a skull project
```shell
skull create project
cd project
skull workflow -add # then input $concurrent, $idl_name, $port
skull module -add # then input $module_name, $workflow_index and $language
skull build
skull deploy
skull start
```

**notes:** By default, the new module with a example code which is used for echo-back message

### Play with skull
```console
telnet localhost 7758
Trying ::1...
Trying 127.0.0.1...
Connected to localhost.
Escape character is '^]'.
hello skull
hello skull
have fun :)
have fun :)

```

## How to Create a Service
The following is the example of adding a service **s1**, and then add a api **get** for it.
```console
bash $> skull service -add
service name? s1
which language the service belongs to? (c) c
data mode? (rw-pr | rw-pw) rw-pr
notice: the common/c folder has already exist, ignore it
service [s1] added successfully
bash $> cd src/services/s1
bash $> skull service --api-add
service api name: get
service api access_mode: (read|write) read
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

## Frequent Questions

[1]: https://github.com/finaldie/skull-admin-c
