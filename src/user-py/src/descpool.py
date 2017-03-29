# Python Protobuf Descriptor Pool (Like the protobuf <= 2.6.1, the descriptor_-
#  pool do not contain a .Default() API, to make it backward compatible, we
#  create a simple pool by ourself.

from google.protobuf import descriptor_pool

_DEFAULT = descriptor_pool.DescriptorPool()

def Default():
    return _DEFAULT

