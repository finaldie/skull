#!/usr/bin/env python3

import sys
import os
import getopt
import string
import pprint
import yaml
import hashlib

# global variables
yaml_obj = None
config_name = ""
header_name = ""

# static

HEADER_CONTENT_START = "\
#ifndef SKULLCPP_CONFIG_H\n\
#define SKULLCPP_CONFIG_H\n\
\n\
#include <skull/config.h>\n\
#include <string>\n\
\n\
namespace skullcpp {\n\
\n\
class Config {\n\
private:\n\
    // Make noncopyable\n\
    Config(const Config&) = delete;\n\
    Config(Config&&) = delete;\n\
    Config& operator=(const Config&) = delete;\n\
    Config& operator=(Config&&) = delete;\n\
\n\
public:\n\
    static Config& instance() {\n\
        static Config _instance_%s;\n\
        return _instance_%s;\n\
    }\n\
\n"

HEADER_CONTENT_END = "\
};\n\
\n\
} // End of namespace\n\
\n\
#endif\n\n"

CONSTRUCTOR_SECTION_START = "\
private:\n\
    ~Config() {}\n\
    Config()"

CONSTRUCTOR_SECTION_END = " {}\n\n"

LOAD_START = "\
public:\n\
    void load(const skull_config_t* config) {\n"

LOAD_END = "    }\n\n"

# ==============================================================================
def load_yaml_config():
    global yaml_obj
    global config_name

    yaml_file = open(config_name, 'r')
    yaml_obj = yaml.load(yaml_file, Loader=yaml.FullLoader)

def _generate_constructor():
    content = CONSTRUCTOR_SECTION_START

    if yaml_obj is None:
        content += CONSTRUCTOR_SECTION_END
        return content

    content += " :\n"
    first_one = True
    for name in yaml_obj:
        value = yaml_obj[name]

        # Skip string type
        if type(value) is str:
            continue

        if first_one:
            first_one = False
            content += "        "
        else:
            content += "        ,"

        if type(value) is int:
            content += "%s_(0)\n" % name
        elif type(value) is bool:
            content += "%s_(false)\n" % name
        elif type(value) is float:
            content += "%s_(0.0f)\n" % name
        else:
            print ("Error: Unsupported type, name: %s, type: %s" % (name, type(value)))
            sys.exit(1)

    content += "   " + CONSTRUCTOR_SECTION_END
    return content

def _generate_loading_api():
    content = LOAD_START

    if yaml_obj is None:
        content += LOAD_END
        return content

    for name in yaml_obj:
        value = yaml_obj[name]

        if type(value) is int:
            content += "        %s_ = skull_config_getint(config, \"%s\", 0);\n" % (name, name)
        elif type(value) is bool:
            content += "        %s_ = skull_config_getbool(config, \"%s\", 0);\n" % (name, name)
        elif type(value) is float:
            content += "        %s_ = skull_config_getdouble(config, \"%s\", 0.0f);\n" % (name, name)
        elif type(value) is str:
            content += "        %s_ = skull_config_getstring(config, \"%s\");\n" % (name, name)
        else:
            print ("Error: Unsupported type, name: %s, type: %s" % (name, type(value)))
            sys.exit(1)

    content += LOAD_END
    return content

def _generate_data_members():
    content = "private:\n"

    if yaml_obj is None:
        return content + "\n"

    for name in yaml_obj:
        value = yaml_obj[name]

        if type(value) is int:
            content += "    int %s_;\n" % name
        elif type(value) is bool:
            content += "    bool %s_;\n" % name
        elif type(value) is float:
            content += "    double %s_;\n" % name
        elif type(value) is str:
            content += "    std::string %s_;\n" % name
        else:
            print ("Error: Unsupported name: %s, type: %s" % (name, type(value)))
            sys.exit(1)

    return content + "\n"

def _generate_data_apis():
    content = "public:\n"

    if yaml_obj is None:
        return content

    for name in yaml_obj:
        value = yaml_obj[name]

        if type(value) is int:
            content += "    int %s() const {return %s_;}\n" % (name, name)
        elif type(value) is bool:
            content += "    bool %s() const {return %s_;}\n" % (name, name)
        elif type(value) is float:
            content += "    double %s() const {return %s_;}\n" % (name, name)
        elif type(value) is str:
            content += "    const std::string& %s() const {return %s_;}\n" % (name, name)
        else:
            print ("Error: Unsupported name: %s, type: %s" % (name, type(value)))
            sys.exit(1)

    return content

def generate_header():
    global config_name
    header_file = open(header_name, 'w')
    content = ""

    # generate header
    m = hashlib.md5()
    m.update(config_name.encode('utf-8'))
    md5_of_config = m.hexdigest()
    content += HEADER_CONTENT_START % (md5_of_config, md5_of_config)

    # generate the config body
    #  constructor part
    content += _generate_constructor()

    #  loading api part
    content += _generate_loading_api()

    #  data member part
    content += _generate_data_members()

    #  data api part
    content += _generate_data_apis()

    # generate tailer
    content += HEADER_CONTENT_END

    # write and close the header file
    header_file.write(content)
    header_file.close()

def generate_config():
    generate_header()

def usage():
    print ("usage: skull-config-gen.py -c config -h output_header_file")

if __name__ == "__main__":
    if len(sys.argv) == 1:
        usage()
        sys.exit(1)

    try:
        opts, args = getopt.getopt(sys.argv[1:], 'c:h:')

        for op, value in opts:
            if op == "-c":
                config_name = value
                load_yaml_config()
            elif op == "-h":
                header_name = value

        # Now run the process func according the mode
        generate_config()

    except Exception as e:
        print ("Fatal: " + str(e))
        usage()
        raise
