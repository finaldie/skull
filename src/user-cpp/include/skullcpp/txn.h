#ifndef SKULLCPP_TXN_H
#define SKULLCPP_TXN_H

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <google/protobuf/message.h>

#include <skull/txn.h>

namespace skullcpp {

class Txn {
private:
    // Make noncopyable
    Txn(const Txn&) = delete;
    Txn(Txn&&) = delete;
    Txn& operator=(const Txn&) = delete;
    Txn& operator=(Txn&&) = delete;

public:
    typedef enum Status {
        TXN_OK      = 0,
        TXN_ERROR   = 1,
        TXN_TIMEOUT = 2
    } Status;

    typedef enum IOStatus {
        OK            = 0,
        ERROR_SRVNAME = 1,
        ERROR_APINAME = 2,
        ERROR_STATE   = 3,
        ERROR_BIO     = 4,
        ERROR_SRVBUSY = 5
    } IOStatus;

    typedef enum PeerType {
        NONE  = 0,
        STD   = 1,
        TCPV4 = 2,
        TCPV6 = 3,
        UDPV4 = 4,
        UDPV6 = 5
    } PeerType;

    typedef int (*ApiCB) (Txn&, IOStatus, const std::string& apiName,
                          const google::protobuf::Message& request,
                          const google::protobuf::Message& response);

public:
    Txn() {};
    virtual ~Txn() {};

public:
    virtual google::protobuf::Message& data() = 0;
    virtual Status status() const = 0;

    virtual const std::string& peerName() const = 0;
    virtual int peerPort() const = 0;
    virtual PeerType peerType() const = 0;
    virtual const std::string& peerTypeName(PeerType type) const = 0;

    /**
     * Invoke a service async call (no pending)
     *
     * @param serivce_name
     * @param apiName
     * @param request       request protobuf message
     *
     * @return - OK
     *         - ERROR_SRVNAME
     *         - ERROR_APINAME
     *         - ERROR_STATE
     */
    virtual IOStatus iocall (const std::string& serviceName,
                             const std::string& apiName,
                             const google::protobuf::Message& request) = 0;

    /**
     * Invoke a service async call
     *
     * @param serivce_name
     * @param apiName
     * @param request       request protobuf message
     * @param cb            api callback function
     *
     * @return - OK
     *         - ERROR_SRVNAME
     *         - ERROR_APINAME
     *         - ERROR_STATE
     *
     * @note If cb is NULL, it's a no pending call as well
     */
    virtual IOStatus iocall (const std::string& serviceName,
                             const std::string& apiName,
                             const google::protobuf::Message& request,
                             ApiCB cb) = 0;

    /**
     * Invoke a service async call
     *
     * @param serivce_name
     * @param apiName
     * @param request       request protobuf message
     * @param bioIdx        background io index
     *                      - (< 0)  : random pick up a background io to run
     *                      - (= 0)   : do not use background io
     *                      - (> 0) : run on the index of background io
     * @param cb            api callback function
     *
     * @return - OK
     *         - ERROR_SRVNAME
     *         - ERROR_APINAME
     *         - ERROR_STATE
     *         - ERROR_BIO
     *
     * @note If cb is NULL, it's a no pending call as well
     */
    virtual IOStatus iocall (const std::string& serviceName,
                             const std::string& apiName,
                             const google::protobuf::Message& request,
                             int bioIdx,
                             ApiCB cb) = 0;
};

} // End of namespace

#endif

