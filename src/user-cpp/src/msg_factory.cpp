#include <assert.h>
#include <unordered_map>
#include <set>
#include <mutex>
#include <google/protobuf/descriptor.h>

#include "msg_factory.h"

namespace skullcpp {

using namespace google::protobuf;
typedef std::unordered_map<std::string, const google::protobuf::Message*> FactoryCache;

static pthread_key_t thread_local_key;
static std::set<FactoryCache*> cachePool;
static std::mutex cachePoolMutex;

static
void _cache_destroyer(void* data) {
    FactoryCache* cache = (FactoryCache*)data;
    delete cache;

    std::lock_guard<std::mutex> lock(cachePoolMutex);
    std::set<FactoryCache*>::const_iterator iter = cachePool.find(cache);
    if (iter != cachePool.end()) {
        cachePool.erase(iter);
    }
}

MsgFactory::MsgFactory() {
    pthread_key_create(&thread_local_key, _cache_destroyer);
}

MsgFactory::~MsgFactory() {
    std::lock_guard<std::mutex> lock(cachePoolMutex);

    std::set<FactoryCache*>::const_iterator iter = cachePool.begin();
    for (; iter != cachePool.end();) {
        FactoryCache* cache = *iter;
        delete cache;

        cachePool.erase(iter++);
    }
}

Message* MsgFactory::newMsg(const std::string& protoName) {
    FactoryCache* cache = (FactoryCache*)pthread_getspecific(thread_local_key);
    if (!cache) {
        cache = new FactoryCache();
        if (pthread_setspecific(thread_local_key, cache)) {
            return NULL;
        }

        std::lock_guard<std::mutex> lock(cachePoolMutex);
        cachePool.insert(cache);
    }

    // 1. Find the factory message
    const Message* factoryMsg = NULL;
    FactoryCache::iterator iter = cache->find(protoName);

    if (iter != cache->end()) {
        factoryMsg = iter->second;
    } else {
        // Find the factory message from generated_pool
        // Notes: Find the top 1 Message Descriptor which is the main descriptor
        const FileDescriptor* fileDesc =
            DescriptorPool::generated_pool()->FindFileByName(protoName);
        assert(fileDesc);
        const Descriptor* desc = fileDesc->message_type(0);

        factoryMsg = MessageFactory::generated_factory()->GetPrototype(desc);

        // Store the factory message into thread local cache
        cache->insert(std::make_pair(protoName, factoryMsg));
    }

    // 2. Build a new message from factory message
    return factoryMsg->New();
}

} // End of namespace

