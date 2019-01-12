# Dependency variables
SK_DEPS = metrics flibs skull-ft protobuf libyaml

ifeq ($(disable_jemalloc), false)
SK_DEPS += jemalloc
endif

PB_CONF_ARGS ?= CXXFLAGS="-O2 -g -fPIC -std=c++11 -DNDEBUG"

ifeq ($(minimal_deps), true)
    disable_fast_proto := true
endif

ifeq ($(disable_fast_proto), false)
    PB_CPP_IMPL += --cpp_implementation
    PB_LIB_PATH += export LD_LIBRARY_PATH=../src/.libs
else
    PB_CPP_IMPL =
    PB_LIB_PATH = true
endif

# API variables
APIS := api-cpp
INSTALL-APIS := api-cpp
CLEAN-APIS := api-cpp

ifeq ($(api_py), true)
    APIS += api-py
    INSTALL-APIS += install-api-py
    CLEAN-APIS += clean-api-py
endif
