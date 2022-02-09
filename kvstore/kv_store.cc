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
        gflags::ShowUsageWithFlagsRestrict(argv[0], "kv_server");
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
        auto client = std::make_shared<kvstore::KVClient>(addr);

        for (int i = 0; i < FLAGS_repeat; i++) {
            LOG(INFO) << "Repeat: " << i;
            kvstore::TestPut(client, FLAGS_key_size, FLAGS_val_size, FLAGS_batch_size, FLAGS_variable);
        }
    }

    google::ShutdownGoogleLogging();
}