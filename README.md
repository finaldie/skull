[![Build Status](https://travis-ci.org/finaldie/skull.svg?branch=master)](https://travis-ci.org/finaldie/skull)

skull
=====

Fast to start-up, easy to maintain, high productivity serving framework.

## How to Build
Use Ubuntu14.04 as an example.

### Install dependencies
```
apt-get install valgrind libyaml-dev python-yaml libprotobuf-c0 libprotobuf-c0-dev protobuf-c-compiler

git clone git@github.com:finaldie/skull.git
cd skull
git submodule update --init --recursive
make dep
```

### Build
```
make -j4
```

### Install Skull and its related Scripts
```
make install
```

## A Quick Demo
After you installed skull into your system, you can run the following steps to
create your skull project.

### Create a skull project
```
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
```
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

## Exsiting Modules
Module Name | Descrption
------------|-----------
[Admin Module][1] | This is an example module for showing the metrics

## Frequent Questions

[1]: https://github.com/finaldie/skull-admin-c
