#ifndef SKULL_CONFIG_H
#define SKULL_CONFIG_H

// A simple config object, only support 1D format (key: value)
typedef struct skull_config_t skull_config_t;

skull_config_t* skull_config_create(const char* config_file_name);
void skull_config_destroy(skull_config_t* config);

int    skull_config_getint    (const skull_config_t*, const char* key_name,
                               int default_value);
double skull_config_getdouble (const skull_config_t*, const char* key_name,
                               double default_value);
const char* skull_config_getstring(const skull_config_t*, const char* key_name);

#endif

