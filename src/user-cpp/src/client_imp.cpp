#include "client_imp.h"

namespace skullcpp {

const std::string& ClientImp::name() const {
    return this->name_;
}

int ClientImp::port() const {
    return this->port_;
}

Client::Type ClientImp::type() const {
    return this->type_;
}

const std::string& ClientImp::typeName() const {
    static const std::string peerTypeNames[] = {"NONE", "STD", "TCPV4", "TCPV6", "UDPV4", "UDPV6"};
    static const std::string unknownPeerType = "UNKNOWN";

    if (this->type_ < Client::Type::NONE || this->type_ > Client::Type::UDPV6) {
        return unknownPeerType;
    }

    return peerTypeNames[this->type_];
}

} // End of namespace

