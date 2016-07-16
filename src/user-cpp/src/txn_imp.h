#ifndef SKULLCPP_TXN_IMP_H
#define SKULLCPP_TXN_IMP_H

#include "skullcpp/txn.h"

namespace skullcpp {

class TxnImp : public Txn {
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
    TxnImp(skull_txn_t*);
    TxnImp(skull_txn_t*, bool destroyRawData);
    virtual ~TxnImp();

    google::protobuf::Message& data();
    Status status();

    /**
     * Invoke a service async call
     *
     * @param serivce_name
     * @param apiName
     * @param request       request protobuf message
     * @param bio_idx       background io index
     *                      - (-1)  : random pick up a background io to run
     *                      - (0)   : do not use background io
     *                      - (> 0) : run on the index of background io
     * @param cb            api callback function
     *
     * @return - OK
     *         - ERROR_SRVNAME
     *         - ERROR_APINAME
     *         - ERROR_BIO
     */
    IOStatus serviceCall (const std::string& serviceName,
                          const std::string& apiName,
                          const google::protobuf::Message& request,
                          int bio_idx,
                          ApiCB cb);

    virtual IOStatus serviceCall (const std::string& serviceName,
                          const std::string& apiName,
                          const google::protobuf::Message& request,
                          ApiCB cb);

public:
    skull_txn_t* txn();
};

}

#endif

