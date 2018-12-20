* 2018-12-17 1.3.3
  * Engine
    * **Enhancement:** Make cron timer more accurate
  * MISC
    * **Enhancement:** Upgrade `flibs` to v1.2.2
* 2018-12-05 1.3.2
  * Engine
    * **New:** Setup cron job to dump mem stats
    * **Fix:** Fix crash when exiting if tracing enabled
    * **Fix:** Fix mem stat usage for `realloc`
    * **Enhancement:** Switch mem trace log to diag.log instead of stderr
    * **Enhancement:** Refine `sk_malloc` to make it independently
    * **Enhancement:** Split atomic APIs into `sk_atomic`
  * CLI
    * **New:** `skull-trace` generates reports for stats summary, cross-scope and leaking
  * MISC
    * **Fix:** Fix valgrind on alpine-3.8
    * **Enhancement:** Upgrade `protobuf` to v3.6.1
* 2018-08-05 1.3.1
  * Engine
    * **Fix:** Inaccurate memory stat in `ep_send`
  * MISC
    * **Enhancement:** Fix typos
* 2018-07-25 1.2.3
  * Engine
    * **Fix:** Solve memory API re-entrance issue
    * **Enhancement:** Refine _admin_ `memory` command output (more clear)
    * **Enhancement:** Refine _admin_ `info` command output (more clear)
  * CLI
    * **New:** Memory realtime tracing tool `skull trace`
  * MISC
    * **Enhancement:** Cleanup makefile templates
* 2018-07-24 1.2.2
  * Engine
    * **Fix:** Memory stat measurement inaccurate issue in endpoint and service job
  * CLI
    * **New:** New `skull-config` for making makefiles clearly
    * **Enhancement:** Won't create `.skull/makefiles` folder
  * MISC
    * **New:** `memory detail|full` available in _Admin Module_
    * **Enhancement:** Won' copy `.supp` files to `project/bin` folder
* 2018-07-19 1.2.1
  * Engine
    * **Enhancement:** Remove protobuf-c from dependency
  * API
    * **New:** Upgrade python2 to python3
  * CLI
    * **New:** Upgrade python2 to python3
  * MISC
    * **Enhancement:** Move all `_BSD_SOURCE` to `_DEFAULT_SOURCE`
* 2018-06-27 1.1.3
  * Engine
    * **New:** override libc malloc to better measure memory stats
    * **Fix:** txnlog and slowlog can be present in the same transaction
    * **Enhancement:** _Admin Module_ parses commands into an array, more flexible for subcommands
  * MISC
    * **Enhancement:** Upgrade jemalloc to 5.0.1
    * **Enhancement:** Upgrade travis to ubuntu.trusty
* 2017-12-07 1.1.2
  * Engine
    * **New:** Add `Slowlog` option in global config with microsecond precision 
  * CLI
    * **Fix:** Fix a unbreak issue in `skull config` command
* 2017-08-23 1.1.1
  * Engine
    * **New:** Docker integrated
    * **New:** Add `command_bind` config item
    * **Fix:** Fix FT Makefile helper message
    * **Fix:** Fix epoll-loop issue when `concurrency=0`
    * **Enhancement:** Remove all the `__WORDSIZE` macro
    * **Enhancement:** Compatible with `musl`
    * **Enhancement:** Reorder Makefile dependency, make build easier
    * **Enhancement:** Compatible with `alpine` Linux release
    * **Enhancement:** Rename `trigger` to `driver`
  * API
    * **Fix:** Fix a unstable UT failure issue in alpine
  * CLI
    * **Fix:** Config name change from concurrent to concurrency
    * **Enhancement:** Refine output message of creating workflow
    * **Enhancement:** Refine output message of creating module
    * **Enhancement:** Refine output message of creating service
    * **Enhancement:** Make CLI more portable
    * **Enhancement:** Add `musl.supp` valgrind suppression file
    * **Enhancement:** For skull-start, to allow not only inside a skull project
    * **Enhancement:** Workflow port bind to 0.0.0.0 by default
    * **Enhancement:**
  * MISC
    * **New:** Support `--no-log-rolling` command arg
    * **New:** Support logs forward to stdout `--std-forwarding`
    * **Fix:** Fix FT unstable issue in docker
    * **Enhancement:** Refine Makefile targets
    * **Enhancement:** Refine config comments
    * **Enhancement:** Upgrade `flibs` to v1.0
    * **Enhancement:** Upgrade `skull-ft` to latest version
    * **Enhancement:** Remove checking `clean` target in `skull build`
* 2017-07-03 1.0
  * Version 1.0 release
* 2017-06-21 1.0-rc3
  * Core
    **Enhancement:** Output more details when logger cannot be created
  * User
    **Enhancement:** Fix dead-lock when exception occurred in `pack` phase
* 2017-06-12 1.0-rc2
  * Core
    * **Enhancement:** Check module.init/service.init return value
  * User
    * **Enhancement:** Better error handling when exception occurred
    * **Enhancement:** Python layer support dumping the stacktrace
    * **Enhancement:** Use Client object instead of peer_xxx apis
    * **Enhancement:** Make `pack` function error handling robuster
  * MISC
    * **Enhancement:** Upgrade to latest `skull-ft`
* 2017-04-14 1.0-rc1
  * Version 1.0 first release candidate
* 2017-04-13 0.9.16
  * Core
    * **New:** Dump memory stats when static linked with jemalloc
  * MISC
    * **Enhancement:** Upgrade jemalloc to 4.5.0
* 2017-04-11 0.9.15
  * Core
    * **Fix:** Fix admin output for `module_list` part
    * **Enhancement:** Add more logs during starting phase for a better experience
    * **Enhancement:** Reorder dynamic counter output format
  * User
    * **New:** Add `peer_xxx` APIs
  * MISC
    * **Enhancement:** Upgrade flibs to latest version
    * **Enhancement:** Lock down protobuf version to 2.6.1
    * **Enhancement:** Cleanup useless files
* 2016-12-17 0.9.14
  * Core
    * **Enhancement:** Optimize the timer service, reduce cpu usage
    * **Enhancement:** Increase admin response buffer size
* 2016-12-15 0.9.13
  * Core
    * **New:** Support IPv6 for client entity and endpoint entity
    * **Fix:** Fix UDP entity cannot be routed to worker io issue
    * **Enhancement:** Refine the sk_sched_send api
  * Scripts
    * **Fix:** Correct return code for 'skull deploy'
  * MISC
    * **Fix:** Prevent errors for 'clean-jemalloc' target if the makefile non-exist
    * **Enhancement:** Upgrade jemalloc to 4.4.0
    * **Enhancement:** Fix typos and add skull-engine binary into gitignore file
* 2016-11-29 0.9.12
  * Core
    * **Fix:** Fix the memory issue when the txn log is too long (>256 bytes)
    * **Fix:** Fix nopending read/write service job be triggered incorrectly
    * **Enhancement:** Refine admin output and logs
    * **Enhancement:** Disable txn logging by default
  * Scripts
    * **Enhancement:** Refine skull-config.yaml format
  * User
    * **Enhancement:** Refine _Python_ module templates and init files
    * **Enhancement:** _Python_ module unpack/pack functions are optional
* 2016-11-13 0.9.11
  * Core
    * **New:** Add UDP Trigger
    * **New:** Entity can be auto cleanup
    * **Enhancement:** Rename sk_trigger_sock to sk_trigger_tcp
    * **Enhancement:** Refactor entity type
    * **Enhancement:** Workflow config, rename 'bind4' to 'bind'
  * Scripts
    * **Fix:** Fix service importing failure issue
    * **Enhancement:** Better format of generating 'skull-config.yaml'
  * Test
    * **New:** Add `httpclient` FT case
  * MISC
    * **Enhancement:** Only search for the top level Makefiles for building the common/module/service
* 2016-10-30 0.9.10
  * Core
    * **Fix:** Fix padded issue in 32bit platform. Compatible with `Raspberry Pi`
  * MISC
    * **Enhancement:** Upgrade flibs to 0.9.4
* 2016-10-24 0.9.9
  * Core
    * **Fix:** Fix crash issue for stdin and immediately triggers
  * Scripts
    * **Enhancement:** Make better user experience for `skull workflow -add`
* 2016-10-23 0.9.8
  * MISC
    * **Enhancement:** Fix Typos
* 2016-10-22 0.9.7
  * Core
    * **Fix:** Refactor ep.unpack api return value type, `size_t` -> `ssize_t`
  * Scripts
    * **Fix:** Fix some command output
  * User
    * **Fix:** Refactor cpp ep.unpack api return value type, `size_t` -> `ssize_t`
    * **Fix:** Force link `common-lib` in cpp module
* 2016-10-17 0.9.6
  * Core
    * **Enhancement:** Enhance 'info' admin command to expose more information
* 2016-10-12 0.9.5
  * Core
    * **Fix:** Refactor unpack api return value type, `size_t` -> `ssize_t`
  * User
    * **New:** Add _Python_ Http Handler (Integrated with Nginx)
    * **New:** Refactor _Cpp_ layer `skull-metrics-gen.py`
    * **Enhancement:** _Python_ APIs robuster
    * **Enhancement:** _Cpp_ APIs robuster
  * Test
    * **Enhancement:** Add an example FT case
* 2016-09-27 0.9.4
  * User
    * **New:** Add _Python_ API layer
    * **Fix:** Remove `ServiceApiReqRawData` structure, make module and service standalone
    * **Fix:** Fix some namespace issue for cpp/py in lower version of compiler
  * Test
    * **New:** Add _Python_ API layer FT cases
  * Scripts
    * **Fix:** User can create python module now
    * **Fix:** Fix common name issue
* 2016-08-26 0.9.3
  * Scripts
    * **New:** Integrate `skull-ft`
    * **Enhancement:** Add some reminder message for workflow actions, to show where are the idl files
    * **Enhancement:** Build proto files for workflow/service together
    * **Enhancement:** Refactor skull_utils.bash
* 2016-08-19 0.9.2
  * Core
    * **New:** Add 'bind4' workflow config item, by default is 127.0.0.1 for the security reason
    * **New:** Add workflow 'timeout' config item, for controlling the timeout case
    * **Enhancement:** Rename skull_service_async_call to skull_txn_iocall
    * **Enhancement:** Make service.create_job apis robuster
    * **Enhancement:** Make ep nopending api robuster
    * **Enhancement:** Make txn.iocall robuster, it will failed if be called from module.unpack
  * User
    * **Enhancement:** Rename txn.serviceCall to txn.iocall for cpp api
* 2016-08-15 0.9.1
  * Core
    * **Fix:** Do not calling service api/job if service is busy
    * **Fix:** Fix a potential memleak in txn.iocall if callback is NULL
    * **Enhancement:** Add an optional service config item for controlling max queue size
  * User
    * **Enhancement:** Upgrade service/txn apis for handling service busy case
* 2016-08-12 0.8.11
  * Core
    * **Fix:** Resolve ep pool crash issue
    * **Fix:** Add missing metrics when ep client be destroyed
    * **Fix:** Timer entity leak issue
    * **Enhancement:** Add Shutdown timer for ep client
    * **Enhancement:** Set thread name align with thread_env.name
    * **Enhancement:** Support config 'max_fds' item
    * **Enhancement:** Ingore SIGPIPE signal for entire application
  * User
    * **Enhancement:** Performance improve for protobuf reflection
  * Scripts
    * **Enhancement:** Add `ulimit -c unlimited` in skull-start script
* 2016-07-20 0.8.10
  * Core
    * **Enhancement:** Make skull easier to be built on other Linux releases
    * **Enhancement:** Upgrade `flibs` and `skull-ft` to latest version
  * User
    * **Enhancement:** Get more information from EPClientRet
* 2016-07-18 0.8.9
  * User
    * **Enhancement:** Make apis robuster
* 2016-07-15 0.8.8
  * Core
    * **Enhancement:** C api layer support pending service job creation
  * User
    * **Fix:** Make `ServiceApiData` non-copyable
    * **Enhancement:** Make `Txn` apis robuster and easier to use
    * **Enhancement:** CPP api layer support pending service job creation
  * Test
    * **New:** Add FT case for test pending service job
* 2016-07-13 0.8.7
  * Core
    * **Enhancement:** C api layer support invoking no pending ep call
  * User
    * **Enhancement:** CPP api layer support invoking no pending ep call
  * Test
    * **New:** Add FT case for test no pending ep call
* 2016-07-11 0.8.6
  * Core
    * **New:** Add a new api `sk_config_getbool`
  * User
    * **Enhancement:** Refactor cpp config generator
    * **Enhancement:** Make cpp apis more strictly
  * Script
    * **Fix:** Fix skull-start errors when run outside a skull project
    * **Enhancement:** Re-generate all the configs before building
  * Test
    * **New:** Add FT case for `dns-service` regression test
* 2016-07-07 0.8.5
  * Core
    * **New:** Enable `jemalloc` by default
* 2016-07-06 0.8.4
  * Core
    * **New:** Add txn log into skull.log
    * **Enhancement:** Refine some `sk_txn` apis
    * **Enhancement:** Downgrade some error logs to trace level in skull.core
* 2016-06-28 0.8.3
  * Core
    * **Fix:** Endpoint leaking issue
    * **Fix:** Crash when query service api with writing access
    * **Fix:** Typos in code
    * **Enhancement:** Endpoint api support to pass the flags arg
  * User
    * **New:** Add skullcpp/logger.h
    * **Enhancement:** Refine examples
    * **Enhancement:** Refine service.set/get, automatically destory the data
  * Script
    * **New:** Support start a daemon mode skull-engine
    * **Fix:** Fix main makefile to compatible with more OS releases
    * **Enhancement:** Correct pass the exit code when run start or build action
  * Test
    * **New:** Add 17 FT cases to cover major use cases
* 2016-04-11 0.8.2
  * Script
    * **Enhancement:** Support deploying to a specific absolute path
* 2016-04-11 0.8.1
  * Core
    * **Enhancement:** Command port can be configurable
    * **Enhancement:** Server Status changes, SERVING change to RUNNING
  * Script
    * **Enhancement:** Refine some prompts
* 2016-04-06 0.7.5
  * Core
    * **Fix:** Fix service.data_set cannot work in init/release period
    * **Enhancement:** Remove api section from main config
    * **Enhancement:** Split ServiceApi to ServiceReadApi and ServiceWriteApi
    * **Enhancement:** Refactor service registration api
  * User
    * **Fix:** Fix crash issue in unittest-c
    * **Fix:** Fix EPClient potential crash issue
    * **Fix:** Fix memleak and crash issue when run service unittest
    * **Enhancement:** Move skullcpp::ServiceCall to Txn.serviceCall
    * **Enhancement:** Remove skullcpp metrics_util.h
    * **Enhancement:** Make cpp APIs noncopyable
    * **Enhancement:** Refactor skullcpp::EPClient APIs, make it more user friendly and flexible
    * **Enhancement:** Refactor Service::createJob API, make it more user friendly and flexible
  * Script
    * **Enhancement:** Update gitigore files
* 2016-03-30 0.7.4
  * User
    * **New:** Build a C++ api layer on top of C api layer
  * Script
    * **Enhancement:** Generate more user friendly service proto
* 2016-01-27 0.7.3
  * Core
    * **New:** `EndPoint` support UDP protocol
    * **Enhancement:** service job accept 0 delayed task, which would be scheduled immediately
    * **Enhancement:** `EndPoint` can be called recursively
  * Scripts:
    * **Fix:** Fix incorrect generating service proto api header script
* 2016-01-19 0.7.2
  * Core
    * **New:** Add `EndPoint` component and related api
    * **Enhancement:** Upgrade `flibs` to 0.8.11
* 2016-01-04 0.7.1
  * Core
    * **New:** Make workflow cancelable
    * **Fix:** Service iocall/timer memleak
    * **Fix:** Invalid memory issue when clean up timers during shutdown
    * **Fix:** Service api call chain crash issue
* 2016-01-03 0.6.6
  * Core
    * **New:** Add `last` admin command to show the latest snapshot
    * **New:** Add `status` admin command
    * **New:** Add `bio` engine which is used for execuating low async/low priority tasks
    * **Enhancement:** Refactor service timer logic
    * **Enhancement:** Reduce memory usage
    * **Enhancement:** Add engine name in log cookie
    * **Enhancement:** txn api callback name refactor
    * **Enhancement:** workflow won't be blocked if api call without a callback function
    * **Enhancement:** Refactor sk_pto table, make the priority field easy to adjust
    * **Enhancement:** Update `flibs` to 0.8.9
    * **Fix:** Memleak in unfinished txn or timer
  * Script
    * **Enhancement:** Display modules better
  * User
    * **Enhancement:** Remove the log templates
    * **Enhancement:** txnsharedata/api protos moved to subfolder `protos`
* 2015-12-28 0.6.5
  * Core
    * **New:** Add two internal timers for update metrics
    * **New:** Add sk_mon_snapshot_xx apis
    * **New:** Import `AdminModule`, right now the metrics would be exposed by port 7759
    * **Fix:** Correct timer metrics
    * **Fix:** Make stdin fd nonblocking
    * **Fix:** Upgrade `flibs` to 0.8.6, which fixed few memory issues
    * **Enhancement:** Add a orphan entity manager, a new entity will be there first
* 2015-12-21 0.6.4
  * Core
    * **New:** Support stdin trigger
    * **New:** Add entity metrics and correct connection metrics
    * **Enhancement:** Refine timer service apis
  * Scripts
    * **Fix:** Fix errors when there is no service exist
* 2015-12-10 0.6.3
  * Scripts
    * **New:** Add skull-service-import which easy to import a new service
    * **New:** Add skull-config command
    * **Enhancement:** Refine skull-config-utils.py
* 2015-12-08 0.6.2
  * Core
    * **Fix:** Fix timer entity memleak
  * Scripts
    * **Enhancement:** Show workflow/modules/services directly when no parameter
* 2015-12-07 0.6.1
  * Core
    * **Fix:** Fix service timer memleak issue
  * User API
    * **Enhancement:** Service timer support user parameter
* 2015-11-15 0.5.5
  * Core
    * **New:** Add 'Timer Service'
    * **Fix:** Fix 'service' data api bugs
    * **Enhancement:** Refine few internal api names
  * Scripts
    * **Enhancement:** Remove 'read-write' data mode
  * User API
    * **New:** 'Timer Service' user apis
    * **Enhancement:** Mock Service data apis
* 2015-10-20 0.5.4
  * Core
    * **New:** Add 'Service' Concept
    * **Fix:** Fix some memleak and crash issues
    * **Enhancement:** Refine some core apis
  * Scripts:
    * **New:** Add commands for service
    * **Enhancement:** Made more user friendly for workflow and module commands
  * User API
    * **New:** Add apis for service
    * **Fix:** Fix some memleak and crash issues
    * **Enhancement:** Refine some user apis
* 2015-03-02 0.5.3
  * User API
    * **Enhancement:** Refine the 'module_pack' arg list -<br>
        add skull_txndata_t for appending the txn data,<br>
        so that it can avoid the user to append the txn data before 'module_pack'
* 2015-03-02 0.5.2
  * Core
    * **New:** Add 'module_release' callback for module
    * **Fix:** Load the config file when a module be loaded into skull
    * **Fix:** Fix a memory leak when module unloading
    * **Enhancement:** Refine many internal api names
  * Scripts
    * **Enhancement:** Hide useless output for top Makefile
  * User API
    * **Enhancement:** Add gitignore for common and module
* 2015-02-26 0.5.1
  * **New:** Add unit test support for C module
  * **New:** Add gitignore and ycm config when 'skull create'
  * **Fix:** Potential crash of C module executor
  * **Enhancement:** Refine part of command output
  * **Enhancement:** Upgrade submodule 'flibs' to 0.8.1
* 2015-02-11 0.4.4
  * **Enhancement:** No longer copy the skull changelog.md to a new project
  * **Enhancement:** Move the user makefiles to .skull/makefiles
* 2015-02-10 0.4.3
  * **New:** Add IDL for module shared data
  * **New:** Add `skull common` actions
  * **New:** Add `skull start --memcheck`
  * **Fix:** Graceful shutdown skull engine
  * **Enhancement:** Split part of user-c loader logic to module executor
* 2015-01-25 0.4.2
  * **New:** Add module configure file support (C API)
  * **Fix:** Fix the workflow_run missing `pack` data issue
* 2015-01-18 0.4.1
  * Add sk_triggers both for passive and proactive type
  * Rename the net_proc protocol to workflow_run
  * Fix the clean-dep
  * Fix `skull module -add` cannot be ran except the top of project
  * Fix the coredump in proactive trigger and workflow_run
* 2015-01-13 0.3.3
  * Integrate with flibs 0.7.4
* 2015-01-09 0.3.2
  * Integrate with travis CI
* 2015-01-09 0.3.1
  * **user:** Add `skull/txn.h` instead of `skull/sk_txn.h`
  * **user:** Fix add module folders incorrect issue
  * **user:** Fix add common folder failure issue due to set language incorrectly
  * **user:** Refactor user-c folder structure
* 2015-01-06 0.2.5
  * replace skull_sched_t with sk_engine_t
  * replace skull_core_t with sk_core_t
* 2015-01-03 0.2.3
  * re-structure the local deployment folder structure, make it cleaner
* 2014-12-28 0.2.2
  * **user:** Rename `components` folder to `src`
  * **user:** Fix few config and makefile issues
* 2014-12-28 0.2.1
  * Add skull metrics apis
  * Add skull user metrics apis
  * Add metrics generator both for engine and user
  * Done the integration with skull-admin-c module
* 2014-11-11 0.2.0
  * Initialization 0.2.0
* 2014-11-07 0.1.2
  * Upgrade flibs to 0.6.5
* 2014-11-06 0.1.1
  * Integrate with flog, and add user log apis
* 2014-09-14 0.1.0
  * Initialization version with:
    * skull-engine basic framework
    * skull scripts for create/workflow/module/build/deploy/start actions
