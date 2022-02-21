#ifndef GRPC_KVSTORE_KV_SERVER_H
#define GRPC_KVSTORE_KV_SERVER_H

#include <utility>
#include <grpcpp/server_builder.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include "glog/logging.h"
#include "rocksdb/db.h"
#include "flags.h"
#include "kvstore.grpc.pb.h"
#include "common.h"


namespace kvstore {
    /*
    class CallData {
    public:
        CallData() = default;

        virtual ~CallData() = default;

        virtual void Proceed() = 0;
    };

    template<typename Request, typename Response>
    class RocksdbCall final : public CallData {
    public:
        RocksdbCall(rocksdb::DB *db,
                    KVStore::AsyncService *service, grpc::ServerCompletionQueue *cq)
                : db_(db), service_(service), cq_(cq), responder_(&ctx_), status_(CREATE) {
            Proceed();
        }

        void Proceed() override {
            if (status_ == CREATE) {
                status_ = PROCESS;
                if (typeid(Request) == typeid(GetReq)) {
                    service_->RequestGet(&ctx_, &request_, &responder_, cq_, cq_,
                                         this);
                } else if (typeid(Request) == typeid(GetBatchReq)) {
                    service_->RequestGetBatch(&ctx_, &request_, &responder_, cq_, cq_,
                                              this);
                } else if (typeid(Request) == typeid(PutReq)) {
                    service_->RequestPut(&ctx_, &request_, &responder_, cq_, cq_,
                                         this);
                }
            } else if (status_ == PROCESS) {
                new RocksdbCall<Request, Response>(db_, service_, cq_);
                grpc::Status status;
                if (typeid(Request) == typeid(GetReq)) {
                    auto &key = request_->key();
                    std::string *value = reply_->mutable_value();
                    auto s = db_->Get(rocksdb::ReadOptions(), key, value);

                    status = wrapStatus(s, reply_->mutable_status());
                } else if (typeid(Request) == typeid(GetBatchReq)) {
                    // FIXME
                    auto *it = db_->NewIterator(rocksdb::ReadOptions());
                    size_t batch_size = std::numeric_limits<size_t>::max();
                    if (request_->has_limit()) {
                        batch_size = request_->limit();
                    }

                    for (it->SeekToFirst(); it->Valid() && batch_size > 0; it->Next(), batch_size--) {
                        GetBatchResp resp;
                        resp.mutable_kv()->mutable_key()->assign(it->key().data(), it->key().size());
                        resp.mutable_kv()->mutable_value()->assign(it->value().data(), it->value().size());
                        responder_->Write(resp);
                    }
                    CHECK(it->status().ok()) << it->status().ToString();
                    delete it;
                    return grpc::Status::OK;
                }
                else if (typeid(Request) == typeid(PutReq)) {}

                status_ = FINISH;
                responder_.Finish(reply_, status, this);
            } else {
                GPR_ASSERT(status_ == FINISH);
                delete this;
            }
        }

    private:
        rocksdb::DB *db_;
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
        explicit KVStoreServiceImpl(rocksdb::DB *db, bool dry_run) : db_(db), dry_run_(dry_run) {

        }

        ::grpc::Status
        Get(::grpc::ServerContext *context, const ::kvstore::GetReq *request, ::kvstore::GetResp *response) override {
            auto &key = request->key();
            std::string *value = response->mutable_value();
            rocksdb::Status s;
            if (dry_run_) {
                value->resize(request->val_size());
                s = rocksdb::Status::OK();
            } else {
                s = db_->Get(rocksdb::ReadOptions(), key, value);
            }

            return wrapStatus(s, response->mutable_status());
        }

        ::grpc::Status
        Put(::grpc::ServerContext *context, const ::kvstore::PutReq *request, ::kvstore::PutResp *response) override {
            auto &kv = request->kv();
            rocksdb::Status s;
            if (dry_run_) {
                s = rocksdb::Status::OK();
            } else {
                s = db_->Put(rocksdb::WriteOptions(), kv.key(), kv.value());
            }
            return wrapStatus(s, response->mutable_status());
        }

        ::grpc::Status Delete(::grpc::ServerContext *context, const ::kvstore::DeleteReq *request,
                              ::kvstore::DeleteResp *response) override {
            auto &key = request->key();
            rocksdb::Status s;
            if (dry_run_) {
                s = rocksdb::Status::OK();
            } else {
                s = db_->Delete(rocksdb::WriteOptions(), key);
            }
            return wrapStatus(s, response->mutable_status());
        }

        ::grpc::Status GetBatch(::grpc::ServerContext *context, const ::kvstore::GetBatchReq *request,
                                ::grpc::ServerWriter<::kvstore::GetBatchResp> *writer) override {
            size_t batch_size = request->limit();

            if (dry_run_) {
                for (size_t i = 0; i < batch_size; i++) {
                    GetBatchResp resp;
                    auto max_val_size = request->val_size();
                    auto val_size = request->variable() ? random(1, max_val_size) : max_val_size;

                    resp.mutable_kv()->mutable_key()->resize(request->key_size());
                    resp.mutable_kv()->mutable_value()->resize(val_size);
                    writer->Write(resp);
                }
            } else {
                auto *it = db_->NewIterator(rocksdb::ReadOptions());

                for (request->has_start() ? it->Seek(request->start()) : it->SeekToFirst();
                     it->Valid() && batch_size > 0; it->Next(), batch_size--) {
                    GetBatchResp resp;
                    resp.mutable_kv()->mutable_key()->assign(it->key().data(), it->key().size());
                    resp.mutable_kv()->mutable_value()->assign(it->value().data(), it->value().size());
                    writer->Write(resp);
                }
                CHECK(it->status().ok()) << it->status().ToString();
                delete it;
            }
            return grpc::Status::OK;
        }

        ::grpc::Status
        GetBigKV(::grpc::ServerContext *context, const ::kvstore::BigGetReq *request,
                 ::kvstore::BigGetResp *response) override {
            auto &key = request->key();
            response->mutable_value()->resize(request->val_size());
            return wrapStatus(rocksdb::Status::OK(), response->mutable_status());
        }

    private:
        rocksdb::DB *db_;
        bool dry_run_;

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

            sync_service_ = std::make_unique<KVStoreServiceImpl>(db_, FLAGS_dry_run);
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
        /*
        void HandleRpcs() {
            new CallData<GetReq, GetResp>(db_, &async_service_, cq_.get());
            new CallData<GetBatchReq, GetBatchResp>(db_, &async_service_, cq_.get());
            new CallData<PutReq, PutResp>(db_, &async_service_, cq_.get());
            void *tag;
            bool ok;
            while (true) {
                GPR_ASSERT(cq_->Next(&tag, &ok));
                GPR_ASSERT(ok);
                static_cast<CallData>
            }
        }
         */
    };
}
#endif //GRPC_KVSTORE_KV_SERVER_H
