#ifndef GRPC_KVSTORE_KV_SERVER_H
#define GRPC_KVSTORE_KV_SERVER_H

#include <utility>
#include <grpcpp/server_builder.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include "glog/logging.h"
#include "rocksdb/db.h"
#include "flags.h"
#include "kvstore.grpc.pb.h"


namespace kvstore {
    /*
    template<typename Request, typename Response>
    class CallData {
    public:
        CallData(KVStore::AsyncService *service, grpc::ServerCompletionQueue *cq)
                : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE) {
            Proceed();
        }

        void Proceed() {
            if (status_ == CREATE) {
                status_ = PROCESS;
                service_->RequestGet(&ctx_, &request_, &responder_, cq_, cq_,
                                     this);
            } else if (status_ == PROCESS) {
                new CallData(service_, cq_);
                std::string prefix("Hello ");
                reply_.set_message(prefix + request_.name());
                status_ = FINISH;
                responder_.Finish(reply_, grpc::Status::OK, this);
            } else {
                GPR_ASSERT(status_ == FINISH);
                delete this;
            }
        }

    private:
        KVStore::AsyncService *service_;
        grpc::ServerCompletionQueue *cq_;
        grpc::ServerContext ctx_;

        Request request_;
        Response reply_;

        grpc::ServerAsyncResponseWriter<Response> responder_;

        enum CallStatus {
            CREATE, PROCESS, FINISH
        };
        CallStatus status_;
    };
     */

    class KVStoreServiceImpl final : public KVStore::Service {
    public:
        explicit KVStoreServiceImpl(rocksdb::DB *db) : db_(db) {

        }

        ::grpc::Status
        Get(::grpc::ServerContext *context, const ::kvstore::GetReq *request, ::kvstore::GetResp *response) override {
            auto &key = request->key();
            std::string *value = response->mutable_value();
            auto s = db_->Get(rocksdb::ReadOptions(), key, value);

            return wrapStatus(s, response->mutable_status());
        }

        ::grpc::Status
        Put(::grpc::ServerContext *context, const ::kvstore::PutReq *request, ::kvstore::PutResp *response) override {
            auto &key = request->key();
            auto &value = request->value();
            auto s = db_->Put(rocksdb::WriteOptions(), key, value);

            return wrapStatus(s, response->mutable_status());
        }

        ::grpc::Status Delete(::grpc::ServerContext *context, const ::kvstore::DeleteReq *request,
                              ::kvstore::DeleteResp *response) override {
            auto &key = request->key();
            auto s = db_->Delete(rocksdb::WriteOptions(), key);

            return wrapStatus(s, response->mutable_status());
        }

    private:
        rocksdb::DB *db_;

        static grpc::Status wrapStatus(const rocksdb::Status &rdb_status, Status *status) {
            if (rdb_status.ok()) {
                status->set_error_code(ErrorCode::OK);
            } else {
                status->set_error_code(ErrorCode::SERVER_ERROR);
                status->set_error_msg(rdb_status.ToString());
            }
            return grpc::Status::OK;
        }
    };

    class KVServer {
    public:
        KVServer(std::string db_file, std::string addr) :
                db_file_(std::move(db_file)),
                addr_(std::move(addr)) {

        }

        void Start() {
            CHECK(db_ == nullptr);
            db_ = createAndOpenDB(db_file_.c_str());

            sync_service_ = std::make_unique<KVStoreServiceImpl>(db_);
            grpc::EnableDefaultHealthCheckService(true);
            grpc::reflection::InitProtoReflectionServerBuilderPlugin();
            grpc::ServerBuilder builder;
            builder.AddListeningPort(addr_, grpc::InsecureServerCredentials());
            builder.RegisterService(sync_service_.get());
            server_ = builder.BuildAndStart();
            LOG(INFO) << "Server is listening on " << addr_;
            server_->Wait();
        }

        void Stop() {
            server_->Shutdown();
            if (db_ != nullptr) {
                closeDB(db_);
            }
        }

    private:
        rocksdb::DB *db_{};
        std::string db_file_;
        std::string addr_;
        std::unique_ptr<grpc::ServerCompletionQueue> cq_;
        std::unique_ptr<KVStoreServiceImpl> sync_service_;
        KVStore::AsyncService async_service_;
        std::unique_ptr<grpc::Server> server_;

        static rocksdb::DB *createAndOpenDB(const char *path) {
            rocksdb::DB *db;
            rocksdb::Options options;
            options.create_if_missing = true;
            rocksdb::Status status = rocksdb::DB::Open(options, path, &db);
            CHECK(status.ok());
            return db;
        }

        static void closeDB(rocksdb::DB *db) {
            auto s = db->Close();
            CHECK(s.ok()) << s.ToString();
            delete db;
        }

    };
}
#endif //GRPC_KVSTORE_KV_SERVER_H
