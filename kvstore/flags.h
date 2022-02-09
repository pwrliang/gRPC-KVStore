#ifndef GRPC_KVSTORE_FLAGS_H
#define GRPC_KVSTORE_FLAGS_H

#include "gflags/gflags_declare.h"

DECLARE_string(db_file);
DECLARE_string(addr);
DECLARE_int32(port);
DECLARE_bool(server);
DECLARE_int32(repeat);
DECLARE_uint32(key_size);
DECLARE_uint32(val_size);
DECLARE_uint32(batch_size);
DECLARE_bool(variable);
DECLARE_string(cmd);
#endif //GRPC_KVSTORE_FLAGS_H
