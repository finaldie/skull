#ifndef SKULLCPP_TXN_H
#define SKULLCPP_TXN_H

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <google/protobuf/message.h>

#include <skull/txn.h>

namespace skullcpp {

class Service;

class Txn {
private:
    google::protobuf::Message* msg_;
    skull_txn_t* txn_;
    bool destroyRawData_;

#if __WORDSIZE == 64
    uint32_t __padding :24;
    uint32_t __padding1;
#else
    uint32_t __padding :24;
#endif

public:
    typedef enum Status {
        TXN_OK = 0,
        TXN_ERROR
    } Status;

    typedef enum IOStatus {
        OK            = 0,
        ERROR_SRVNAME = 1,
        ERROR_APINAME = 2,
        ERROR_BIO     = 3
    } IOStatus;

    typedef int (*ApiCB) (Txn&, const std::string& apiName,
                          const google::protobuf::Message& request,
                          const google::protobuf::Message& response);

public:
    Txn(skull_txn_t*);
    Txn(skull_txn_t*, bool destroyRawData);
    ~Txn();

    google::protobuf::Message& data();
    Status status();

    /**
     * Invoke a service async call
     *
     * @param serivce_name
     * @param apiName
     * @param request       request protobuf message
     * @param cb            api callback function
     * @param bio_idx       background io index
     *                      - (-1)  : random pick up a background io to run
     *                      - (0)   : do not use background io
     *                      - (> 0) : run on the index of background io
     *
     * @return - OK
     *         - ERROR_SRVNAME
     *         - ERROR_APINAME
     *         - ERROR_BIO
     */
    IOStatus serviceCall (const char* serviceName,
                          const char* apiName,
                          const google::protobuf::Message& request,
                          ApiCB cb,
                          int bio_idx);

public:
    skull_txn_t* txn();
};

} // End of namespace

#endif

