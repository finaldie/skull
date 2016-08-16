* 2016-08-15 0.9.1
  * Core
    * **Fix:** Do not calling service api/job if service busy
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
