#ifndef SK_LOG_TPL_H
#define SK_LOG_TPL_H

// log template related apis:
// log template include 3 parts:
//  1. logId
//  2. msg
//  3. solution (optional, only warn|error|fatal level have this)
//
// info log format:
// logid: msg
//
// warn|error|fatal log format:
// logid:
//   msg: xxx
//   solution: xxx
// ...

typedef enum sk_log_tpl_type_t {
    SK_LOG_INFO_TPL = 0,
    SK_LOG_ERROR_TPL
} sk_log_tpl_type_t;

typedef struct sk_log_tpl_t sk_log_tpl_t;

sk_log_tpl_t* sk_log_tpl_create(const char* filename, sk_log_tpl_type_t type);
void sk_log_tpl_destroy(sk_log_tpl_t*);

const char* sk_log_tpl_msg(sk_log_tpl_t*, int log_id);
const char* sk_log_tpl_solution(sk_log_tpl_t*, int log_id);

#endif

