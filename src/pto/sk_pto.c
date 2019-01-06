#include <stdlib.h>

#include "api/sk_pto.h"
#include "api/sk_utils.h"

extern sk_proto_ops_t sk_pto_ops_tcp_accept;
extern sk_proto_ops_t sk_pto_ops_workflow_run;
extern sk_proto_ops_t sk_pto_ops_entity_destroy;
extern sk_proto_ops_t sk_pto_ops_srv_iocall;
extern sk_proto_ops_t sk_pto_ops_srv_task_run;
extern sk_proto_ops_t sk_pto_ops_srv_task_done;
extern sk_proto_ops_t sk_pto_ops_timer_triggered;
extern sk_proto_ops_t sk_pto_ops_timer_emit;
extern sk_proto_ops_t sk_pto_ops_std_start;
extern sk_proto_ops_t sk_pto_ops_svc_timer_run;
extern sk_proto_ops_t sk_pto_ops_svc_timer_done;
extern sk_proto_ops_t sk_pto_ops_srv_task_cb;

sk_proto_t sk_pto_tbl[] = {
    {SK_PTO_TCP_ACCEPT,      SK_PTO_PRI_7, 1, 0, 10, &sk_pto_ops_tcp_accept},
    {SK_PTO_WORKFLOW_RUN,    SK_PTO_PRI_5, 0, 0, 11, &sk_pto_ops_workflow_run},
    {SK_PTO_ENTITY_DESTROY,  SK_PTO_PRI_9, 0, 0, 12, &sk_pto_ops_entity_destroy},
    {SK_PTO_SVC_IOCALL,      SK_PTO_PRI_6, 5, 0, 13, &sk_pto_ops_srv_iocall},
    {SK_PTO_SVC_TASK_RUN,    SK_PTO_PRI_4, 5, 0, 14, &sk_pto_ops_srv_task_run},
    {SK_PTO_SVC_TASK_DONE,   SK_PTO_PRI_6, 3, 0, 15, &sk_pto_ops_srv_task_done},
    {SK_PTO_TIMER_TRIGGERED, SK_PTO_PRI_8, 3, 0, 16, &sk_pto_ops_timer_triggered},
    {SK_PTO_TIMER_EMIT,      SK_PTO_PRI_8, 7, 0, 17, &sk_pto_ops_timer_emit},
    {SK_PTO_STD_START,       SK_PTO_PRI_7, 0, 0, 18, &sk_pto_ops_std_start},
    {SK_PTO_SVC_TIMER_RUN,   SK_PTO_PRI_8, 6, 0, 19, &sk_pto_ops_svc_timer_run},
    {SK_PTO_SVC_TIMER_DONE,  SK_PTO_PRI_8, 3, 0, 20, &sk_pto_ops_svc_timer_done},
    {SK_PTO_SVC_TASK_CB,     SK_PTO_PRI_6, 5, 0, 21, &sk_pto_ops_srv_task_cb},
    {SK_PTO_MAX, 0, 0, 0, 0, NULL}
};

#define SK_PTO_BODY(hdr) ((char*)hdr + sizeof(sk_pto_hdr_t))
#define SK_PTO_ARGSZ sizeof(sk_arg_t)
#define SK_PTO_CHKBITS (0xFFFF)

#define checksum(data, sz, salt) (_checksum(data, sz, salt) & SK_PTO_CHKBITS)

static
uint32_t _checksum(void* data, size_t sz, uint32_t salt) {
    if (!data || !sz) return 0;

    uint32_t checksum = 0;

    char const* c = data;
    for (size_t i = 0; i < sz; i++, c++) {
        uint32_t uc = (uint32_t)(*c) ? (uint32_t)(*c) : 1;
        checksum += uc * salt;
        //sk_print("_checksum: %u, salt: %u, sz: %zu, i: %zu, c: %u, uc: %u\n",
        //         checksum, salt, sz, i, *c, uc);
    }

    return checksum;
}

size_t    sk_pto_pack (uint32_t id, sk_pto_hdr_t* hdr, va_list ap) {
    if (!hdr) return 0;

    sk_proto_t* pto = &sk_pto_tbl[id];
    SK_ASSERT_MSG(id == pto->id, "Mis-matched pto id: %u - %u\n", id, pto->id);
    hdr->id = id & 0xFF;
    hdr->argc = pto->argc;

    sk_arg_t* p = (sk_arg_t*)SK_PTO_BODY(hdr);
    int i = 0;
    for (; i < pto->argc; i++, p++) {
        *p = va_arg(ap, sk_arg_t);
    }

    SK_ASSERT_MSG(i == hdr->argc, "Mis-matched argc when create pto: %u\n", id);

    size_t body_sz = SK_PTO_ARGSZ * (size_t)i;
    uint32_t checksum = checksum(SK_PTO_BODY(hdr), body_sz, pto->salt);
    hdr->chk = checksum & SK_PTO_CHKBITS;
    sk_print("sk_pto_pack: id: %u, argc: %u, rawchk: %u, chk: %u\n",
             hdr->id, hdr->argc, checksum, hdr->chk);
    return sizeof(*hdr) + body_sz;
}

sk_arg_t* sk_pto_arg  (uint32_t id, sk_pto_hdr_t* hdr, int idx) {
    if (!hdr || !hdr->argc) return NULL;
    SK_ASSERT_MSG(id < SK_PTO_MAX, "pto id must < pto max: %u\n", id);
    SK_ASSERT_MSG(id == hdr->id, "pto id mis-match: %u - %u\n", id, hdr->id);

    sk_proto_t* pto = &sk_pto_tbl[hdr->id];
    SK_ASSERT_MSG(idx < pto->argc, "Request pto idx must < argc: %d\n", idx);

    return ((sk_arg_t*)SK_PTO_BODY(hdr)) + idx;
}

bool sk_pto_check(uint32_t id, sk_pto_hdr_t* hdr) {
    if (!hdr || !hdr->argc) return true;
    SK_ASSERT_MSG(id < SK_PTO_MAX, "pto id must < pto max: %u\n", id);
    SK_ASSERT_MSG(id == hdr->id, "pto id mis-match: %u - %u\n", id, hdr->id);

    sk_proto_t* pto = &sk_pto_tbl[hdr->id];
    return
        hdr->chk == checksum(SK_PTO_BODY(hdr), SK_PTO_BODYSZ(hdr), pto->salt);
}

