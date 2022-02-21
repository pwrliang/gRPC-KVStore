#include "kv_server.h"
#include <gflags/gflags.h>
#include <gflags/gflags_declare.h>
#include <glog/logging.h>
#include "kv_client.h"
#include "benchmark.h"

int main(int argc, char *argv[]) {
    FLAGS_stderrthreshold = 0;

    gflags::SetUsageMessage(
            "Usage: " + std::string(argv[0]) + " -help");
    if (argc == 1) {
        gflags::ShowUsageWithFlagsRestrict(argv[0], "kvstore/flags.cc");
        exit(1);
    }
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    gflags::ShutDownCommandLineFlags();

    google::InitGoogleLogging(argv[0]);
    google::InstallFailureSignalHandler();

    auto addr = FLAGS_addr + ":" + std::to_string(FLAGS_port);
    if (FLAGS_server) {
        auto server = std::make_unique<kvstore::KVServer>(FLAGS_db_file, addr);
        server->Start();
    } else {
        auto cmd = FLAGS_cmd;
        auto client = std::make_shared<kvstore::KVClient>(addr);
        auto batch_size = FLAGS_batch_size;
        CHECK_NE(FLAGS_addr, "0.0.0.0") << "give me a valid addr?";
        if (FLAGS_warmup) {
            LOG(INFO) << "Warming up";
            kvstore::Warmup(client, FLAGS_key_size, FLAGS_val_size, FLAGS_variable);
        }

        for (int i = 1; i <= FLAGS_repeat; i++) {
            LOG(INFO) << "Repeat: " << i;

            if (cmd == "put") {
                kvstore::TestPut(client, FLAGS_key_size, FLAGS_val_size, batch_size, FLAGS_variable);
            } else if (cmd == "get_batch") {
                kvstore::TestGetBatch(client, batch_size);
            } else if (cmd == "get") {
                kvstore::TestGet(client, batch_size);
            } else if (cmd == "delete") {
                kvstore::TestDelete(client, batch_size);
            } else if (cmd == "get_big") {
                kvstore::TestBigKV(client, FLAGS_big_kv_in_kb * 1024, FLAGS_big_k, FLAGS_big_v);
            } else {
                LOG(FATAL) << "Bad command: " << cmd;
            }
        }
    }

    google::ShutdownGoogleLogging();
}