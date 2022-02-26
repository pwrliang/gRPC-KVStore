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

        ::grpc::Status GetBatch(::grpc::ServerContext *context, const ::kvstore::GetBatchReq *request,
                                ::grpc::ServerWriter<::kvstore::GetBatchResp> *writer) override {
            size_t batch_size = request->has_limit() ? request->limit() : std::numeric_limits<size_t>::max();
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
        std::unique_ptr<KVStoreServiceImpl> sync_service_;
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
