#ifndef GRPC_KVSTORE_BENCHMARK_H
#define GRPC_KVSTORE_BENCHMARK_H

#include "glog/logging.h"
#include "kv_client.h"
#include "stopwatch.h"

namespace kvstore {
    int random(size_t min, size_t max) { //range : [min, max]
        static bool first = true;
        if (first) {
            srand(time(NULL)); //seeding for the first time only!
            first = false;
        }
        return min + rand() % ((max + 1) - min);
    }

    void TestPut(const std::shared_ptr<KVClient> &kv_cli,
                 size_t key_size, size_t val_size, size_t batch_size,
                 bool variable_value_size) {
        Stopwatch sw;
        std::vector<std::pair<std::string, std::string>> reqs;

        reqs.reserve(batch_size);

        for (size_t i = 0; i < batch_size; i++) {
            std::string key, value;
            key.resize(key_size);
            value.resize(variable_value_size ? random(1, val_size) : val_size);
            reqs.emplace_back(std::make_pair(key, value));
        }

        sw.start();
        for (size_t i = 0; i < batch_size; i++) {
            auto &req = reqs[i];
            auto status = kv_cli->Put(req.first, req.second);
            if (status.error_code() != ErrorCode::OK) {
                LOG(FATAL) << "Put Error: " << status.error_code() << " msg: " << status.error_msg();
            }
        }
        sw.stop();

        LOG(INFO) << "Key size: " << key_size << " Value size: " << val_size << " Batch size: " << batch_size;
        LOG(INFO) << "Time: " << sw.ms() << " ms, avg: " << batch_size / (sw.ms() / 1000) << " kv/s";
    }
}

#endif //GRPC_KVSTORE_BENCHMARK_H
