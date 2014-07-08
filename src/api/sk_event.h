#ifndef SK_EVENT_H
#define SK_EVENT_H

typedef struct sk_event_t {
    void* data;

    int (*run)(void* data);
    void (*destroy)(void* data);
    int  (*end)();
} sk_event_t;

#endif

