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
