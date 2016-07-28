#ifndef SKULLCPP_MSG_FACTORY_H
#define SKULLCPP_MSG_FACTORY_H

#include <string>

#include <google/protobuf/message.h>

namespace skullcpp {

class MsgFactory {
private:
    // Make noncopyable
    MsgFactory(const MsgFactory&) = delete;
    MsgFactory(MsgFactory&&) = delete;
    MsgFactory& operator=(const MsgFactory&) = delete;
    MsgFactory& operator=(MsgFactory&&) = delete;

private:
    MsgFactory() {}
    ~MsgFactory() {}

public:
    static MsgFactory& instance() {
        static MsgFactory __instance_msg_factory;
        return __instance_msg_factory;
    }

public:
    // Create a empty message for a specific proto file
    static google::protobuf::Message* newMsg(const std::string& protoName);
};

} // End of namespace

#endif

