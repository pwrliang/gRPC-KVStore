

#ifndef GRPC_KVSTORE_KV_CLIENT_H
#define GRPC_KVSTORE_KV_CLIENT_H

#include <grpcpp/grpcpp.h>
#include "kvstore.grpc.pb.h"


namespace kvstore {
    class KVClient {
    public:
        explicit KVClient(const std::string &addr) : stub_(
                KVStore::NewStub(grpc::CreateChannel(addr, grpc::InsecureChannelCredentials()))) {
            LOG(INFO) << "Client is trying to connect to " << addr;
        }

        Status GetBatch(std::vector<KV> &kvs, size_t batch_size = std::numeric_limits<size_t>::max()) {
            GetBatchReq req;
            grpc::ClientContext cli_ctx;

            req.set_limit(batch_size);
            auto reader = stub_->GetBatch(&cli_ctx, req);
            GetBatchResp resp;
            Status status;

            while (reader->Read(&resp)) {
                kvs.push_back(resp.kv());
            }

            status.set_error_code(ErrorCode::OK);
            return status;
        }

        Status Get(const std::string &key, std::string &value) {
            GetReq req;
            GetResp resp;
            grpc::ClientContext cli_ctx;

            req.set_key(key);
            auto grpc_status = stub_->Get(&cli_ctx, req, &resp);

            if (grpc_status.ok()) {
                if (resp.status().error_code() == ErrorCode::OK) {
                    value = resp.value();
                }
            } else {
                resp.mutable_status()->set_error_code(ErrorCode::CLIENT_ERROR);
                resp.mutable_status()->set_error_msg(grpc_status.error_message());
            }
            return resp.status();
        }

        Status Put(const std::string &key, const std::string &value) {
            PutReq req;
            PutResp resp;
            grpc::ClientContext cli_ctx;

            *req.mutable_kv()->mutable_key() = key;
            *req.mutable_kv()->mutable_value() = value;

            cli_ctx.set_wait_for_ready(true);
            auto grpc_status = stub_->Put(&cli_ctx, req, &resp);

            if (!grpc_status.ok()) {
                resp.mutable_status()->set_error_code(ErrorCode::CLIENT_ERROR);
                resp.mutable_status()->set_error_msg(grpc_status.error_message());
            }
            return resp.status();
        }

        Status Delete(const std::string &key) {
            DeleteReq req;
            DeleteResp resp;
            grpc::ClientContext cli_ctx;

            req.set_key(key);
            auto grpc_status = stub_->Delete(&cli_ctx, req, &resp);

            if (!grpc_status.ok()) {
                resp.mutable_status()->set_error_code(ErrorCode::CLIENT_ERROR);
                resp.mutable_status()->set_error_msg(grpc_status.error_message());
            }
            return resp.status();
        }

    private:
        std::unique_ptr<KVStore::Stub> stub_;
    };


}

#endif //GRPC_KVSTORE_KV_CLIENT_H
