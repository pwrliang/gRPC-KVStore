#include "flags.h"
#include "gflags/gflags.h"

DEFINE_string(db_file, "/tmp/rocks.db", "");
DEFINE_string(addr, "0.0.0.0", "");
DEFINE_int32(port, 12345, "");
DEFINE_bool(server, false, "is server or client");
DEFINE_int32(repeat, 1, "repeat times");
DEFINE_uint32(key_size, 128, "key size in bytes");
DEFINE_uint32(val_size, 4096, "value size");
DEFINE_uint32(batch_size, 100000, "Batch size");
DEFINE_bool(variable, false, "Random value size");
