#ifndef SKULLCPP_TXN_IMP_H
#define SKULLCPP_TXN_IMP_H

#include "skullcpp/txn.h"

namespace skullcpp {

class TxnImp : public Txn {
private:
    google::protobuf::Message* msg_;
    skull_txn_t* txn_;

    std::string peerName_;
    PeerType    peerType_;
    int         peerPort_;

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
    Status status() const;

    const std::string& peerName() const;
    int peerPort() const;
    PeerType peerType() const;

    /**
     * Invoke a service async call
     *
     * @param serivce_name
     * @param apiName
     * @param request       request protobuf message
     * @param bioIdx        background io index
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
    IOStatus iocall (const std::string& serviceName,
                     const std::string& apiName,
                     const google::protobuf::Message& request,
                     int bioIdx,
                     ApiCB cb);

    IOStatus iocall (const std::string& serviceName,
                     const std::string& apiName,
                     const google::protobuf::Message& request,
                     ApiCB cb);

    IOStatus iocall (const std::string& serviceName,
                     const std::string& apiName,
                     const google::protobuf::Message& request);

public:
    skull_txn_t* txn() const;
};

}

#endif

