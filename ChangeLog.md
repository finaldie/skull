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
