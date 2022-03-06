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

    class KVStoreServiceImpl final : public KVStore::Service {
    public:
        explicit KVStoreServiceImpl(rocksdb::DB *db) : db_(db) {

        }

        ::grpc::Status
        Get(::grpc::ServerContext *context, const ::kvstore::GetReq *request, ::kvstore::GetResp *response) override {
            auto &key = request->key();
            std::string *value = response->mutable_value();
            rocksdb::Status s = db_->Get(rocksdb::ReadOptions(), key, value);

            return wrapStatus(s, response->mutable_status());
        }

        ::grpc::Status
        Put(::grpc::ServerContext *context, const ::kvstore::PutReq *request, ::kvstore::PutResp *response) override {
            auto &kv = request->kv();
            rocksdb::Status s = db_->Put(rocksdb::WriteOptions(), kv.key(), kv.value());
            return wrapStatus(s, response->mutable_status());
        }

        ::grpc::Status Delete(::grpc::ServerContext *context, const ::kvstore::DeleteReq *request,
                              ::kvstore::DeleteResp *response) override {
            auto &key = request->key();
            rocksdb::Status s = db_->Delete(rocksdb::WriteOptions(), key);
            return wrapStatus(s, response->mutable_status());
        }

        ::grpc::Status Scan(::grpc::ServerContext *context, const ::kvstore::ScanReq *request,
                            ::grpc::ServerWriter<::kvstore::ScanResp> *writer) override {
            size_t batch_size = request->has_limit() ? request->limit() : std::numeric_limits<size_t>::max();
            auto *it = db_->NewIterator(rocksdb::ReadOptions());

            for (request->has_start() ? it->Seek(request->start()) : it->SeekToFirst();
                 it->Valid() && batch_size > 0; it->Next(), batch_size--) {
                ScanResp resp;
                resp.mutable_kv()->mutable_key()->assign(it->key().data(), it->key().size());
                resp.mutable_kv()->mutable_value()->assign(it->value().data(), it->value().size());
                writer->Write(resp);
            }
            CHECK(it->status().ok()) << it->status().ToString();
            delete it;
            return grpc::Status::OK;
        }

        ::grpc::Status
        GetBigKV(::grpc::ServerContext *context, const ::kvstore::BigGetReq *request,
                 ::kvstore::BigGetResp *response) override {
            response->mutable_value()->resize(request->val_size());
            return wrapStatus(rocksdb::Status::OK(), response->mutable_status());
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

    enum class CallStatus {
        CREATE, PROCESS, FINISH
    };

    class Call {
    public:
        Call(KVStore::AsyncService *service,
             grpc::ServerCompletionQueue *cq,
             rocksdb::DB *db) : service_(service), cq_(cq), db_(db), call_status_(CallStatus::CREATE) {
        }

        virtual ~Call() = default;

        virtual void Proceed() = 0;

    protected:
        KVStore::AsyncService *service_;
        grpc::ServerCompletionQueue *cq_;
        grpc::ServerContext ctx_;

        rocksdb::DB *db_;
        CallStatus call_status_;

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

    class GetCall : public Call {
    public:
        GetCall(KVStore::AsyncService *service,
                grpc::ServerCompletionQueue *cq,
                rocksdb::DB *db) :
                Call(service, cq, db), responder_(&ctx_) {
            call_status_ = CallStatus::PROCESS;
            service_->RequestGet(&ctx_, &req_, &responder_, cq_, cq_, this);
        }

        void Proceed() override {
            if (call_status_ == CallStatus::PROCESS) {
                new GetCall(service_, cq_, db_);
                auto rocksdb_status = db_->Get(rocksdb::ReadOptions(), req_.key(), resp_.mutable_value());
                call_status_ = CallStatus::FINISH;
                responder_.Finish(resp_, wrapStatus(rocksdb_status, resp_.mutable_status()), this);
            } else {
                GPR_ASSERT(call_status_ == CallStatus::FINISH);
                delete this;
            }
        }

    private:
        GetReq req_;
        GetResp resp_;
        grpc::ServerAsyncResponseWriter<GetResp> responder_;
    };

    class PutCall : public Call {
    public:
        PutCall(KVStore::AsyncService *service,
                grpc::ServerCompletionQueue *cq,
                rocksdb::DB *db) :
                Call(service, cq, db), responder_(&ctx_) {
            call_status_ = CallStatus::PROCESS;
            service_->RequestPut(&ctx_, &req_, &responder_, cq_, cq_, this);
        }

        void Proceed() override {
            if (call_status_ == CallStatus::PROCESS) {
                new PutCall(service_, cq_, db_);
                auto rocksdb_status = db_->Put(rocksdb::WriteOptions(), req_.kv().key(), req_.kv().value());
                call_status_ = CallStatus::FINISH;
                responder_.Finish(resp_, wrapStatus(rocksdb_status, resp_.mutable_status()), this);
            } else {
                GPR_ASSERT(call_status_ == CallStatus::FINISH);
                delete this;
            }
        }

    private:
        PutReq req_;
        PutResp resp_;
        grpc::ServerAsyncResponseWriter<PutResp> responder_;
    };


    class DeleteCall : public Call {
    public:
        DeleteCall(KVStore::AsyncService *service,
                   grpc::ServerCompletionQueue *cq,
                   rocksdb::DB *db) :
                Call(service, cq, db), responder_(&ctx_) {
            call_status_ = CallStatus::PROCESS;
            service_->RequestDelete(&ctx_, &req_, &responder_, cq_, cq_, this);
        }

        void Proceed() override {
            if (call_status_ == CallStatus::PROCESS) {
                new DeleteCall(service_, cq_, db_);
                auto rocksdb_status = db_->Delete(rocksdb::WriteOptions(), req_.key());
                call_status_ = CallStatus::FINISH;
                responder_.Finish(resp_, wrapStatus(rocksdb_status, resp_.mutable_status()), this);
            } else {
                GPR_ASSERT(call_status_ == CallStatus::FINISH);
                delete this;
            }
        }

    private:
        DeleteReq req_;
        DeleteResp resp_;
        grpc::ServerAsyncResponseWriter<DeleteResp> responder_;
    };

    class ScanCall : public Call {
    public:
        ScanCall(KVStore::AsyncService *service,
                 grpc::ServerCompletionQueue *cq,
                 rocksdb::DB *db) :
                Call(service, cq, db), writer_(&ctx_) {
            call_status_ = CallStatus::PROCESS;
            service_->RequestScan(&ctx_, &req_, &writer_, cq_, cq_, this);
        }

        void Proceed() override {
            if (call_status_ == CallStatus::PROCESS) {
                new ScanCall(service_, cq_, db_);

                size_t batch_size = req_.has_limit() ? req_.limit() : std::numeric_limits<size_t>::max();
                auto *it = db_->NewIterator(rocksdb::ReadOptions());

                for (req_.has_start() ? it->Seek(req_.start()) : it->SeekToFirst();
                     it->Valid() && batch_size > 0; it->Next(), batch_size--) {
                    ScanResp resp;
                    resp.mutable_kv()->mutable_key()->assign(it->key().data(), it->key().size());
                    resp.mutable_kv()->mutable_value()->assign(it->value().data(), it->value().size());
                    writer_.Write(resp_, this);
                }
                CHECK(it->status().ok()) << it->status().ToString();
                delete it;
            } else {
                GPR_ASSERT(call_status_ == CallStatus::FINISH);
                delete this;
            }
        }

    private:
        ScanReq req_;
        ScanResp resp_;
        grpc::ServerAsyncWriter<ScanResp> writer_;
    };

    class KVServer {
    public:
        explicit KVServer(const std::string &db_file) {
            db_ = createAndOpenDB(db_file.c_str());
        }

        virtual ~KVServer() {
            if (db_ != nullptr) {
                closeDB(db_);
            }
        }

        virtual void Start() = 0;

        virtual void Stop() = 0;

        rocksdb::DB *get_db() {
            return db_;
        }

    private:
        rocksdb::DB *db_{};

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

    class KVServerSync : public KVServer {
    public:
        KVServerSync(const std::string &db_file, std::string addr) :
                KVServer(db_file),
                addr_(std::move(addr)) {

        }

        void Start() override {
            sync_service_ = std::make_unique<KVStoreServiceImpl>(get_db());
            grpc::EnableDefaultHealthCheckService(true);
            grpc::reflection::InitProtoReflectionServerBuilderPlugin();
            grpc::ServerBuilder builder;
            builder.AddListeningPort(addr_, grpc::InsecureServerCredentials());
            builder.RegisterService(sync_service_.get());
            server_ = builder.BuildAndStart();
            LOG(INFO) << "Sync Server is listening on " << addr_;
            server_->Wait();
        }

        void Stop() override {
            server_->Shutdown();
        }

    private:
        std::string addr_;
        std::unique_ptr<KVStoreServiceImpl> sync_service_;
        std::unique_ptr<grpc::Server> server_;
    };

    class KVServerAsync : public KVServer {
    public:
        KVServerAsync(const std::string &db_file, std::string addr) :
                KVServer(db_file),
                addr_(std::move(addr)) {

        }

        void Start() override {
            grpc::EnableDefaultHealthCheckService(true);
            grpc::reflection::InitProtoReflectionServerBuilderPlugin();
            grpc::ServerBuilder builder;
            builder.AddListeningPort(addr_, grpc::InsecureServerCredentials());
            builder.RegisterService(&service_);
            cq_ = builder.AddCompletionQueue();
            server_ = builder.BuildAndStart();
            LOG(INFO) << "Async Server is listening on " << addr_;
            HandleRpcs();
        }

        void Stop() override {
            server_->Shutdown();
        }

    private:
        std::string addr_;
        KVStore::AsyncService service_;
        std::unique_ptr<grpc::ServerCompletionQueue> cq_;
        std::unique_ptr<grpc::Server> server_;


        void HandleRpcs() {
            auto *db = get_db();

            new GetCall(&service_, cq_.get(), db);
            void *tag;
            bool ok;
            while (true) {
                GPR_ASSERT(cq_->Next(&tag, &ok));
                GPR_ASSERT(ok);
                static_cast<Call *>(tag)->Proceed();
            }
        }
    };
}
#endif //GRPC_KVSTORE_KV_SERVER_H
