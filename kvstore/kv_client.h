

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

            req.set_key(key);
            req.set_value(value);

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
