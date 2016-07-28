#include <unordered_map>
#include <google/protobuf/descriptor.h>

#include "msg_factory.h"

namespace skullcpp {

using namespace google::protobuf;
typedef std::unordered_map<std::string, const google::protobuf::Message*> FactoryCache;

Message* MsgFactory::newMsg(const std::string& protoName) {
    thread_local FactoryCache cache_;

    // 1. Find the factory message
    const google::protobuf::Message* factoryMsg = NULL;
    FactoryCache::iterator iter = cache_.find(protoName);

    if (iter != cache_.end()) {
        factoryMsg = iter->second;
    } else {
        // Find the factory message from generated_pool
        // Notes: Find the top 1 Message Descriptor which is the main descriptor
        const FileDescriptor* fileDesc =
            DescriptorPool::generated_pool()->FindFileByName(protoName);
        const Descriptor* desc = fileDesc->message_type(0);

        factoryMsg = MessageFactory::generated_factory()->GetPrototype(desc);

        // Store the factory message into thread local cache
        cache_.insert(std::make_pair(protoName, factoryMsg));
    }

    // 2. Build a new message from factory message
    return factoryMsg->New();
}

} // End of namespace

