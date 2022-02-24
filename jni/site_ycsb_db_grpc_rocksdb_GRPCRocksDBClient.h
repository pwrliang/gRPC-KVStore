/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class site_ycsb_db_grpc_rocksdb_GRPCRocksDBClient */

#ifndef _Included_site_ycsb_db_grpc_rocksdb_GRPCRocksDBClient
#define _Included_site_ycsb_db_grpc_rocksdb_GRPCRocksDBClient
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     site_ycsb_db_grpc_rocksdb_GRPCRocksDBClient
 * Method:    connect
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_site_ycsb_db_grpc_rocksdb_GRPCRocksDBClient_connect
        (JNIEnv *, jobject, jstring);

/*
 * Class:     site_ycsb_db_grpc_rocksdb_GRPCRocksDBClient
 * Method:    get
 * Signature: (J[B)[B
 */
JNIEXPORT jbyteArray JNICALL Java_site_ycsb_db_grpc_rocksdb_GRPCRocksDBClient_get
        (JNIEnv *, jobject, jlong, jbyteArray);

/*
 * Class:     site_ycsb_db_grpc_rocksdb_GRPCRocksDBClient
 * Method:    put
 * Signature: (J[B[B)I
 */
JNIEXPORT jint JNICALL Java_site_ycsb_db_grpc_rocksdb_GRPCRocksDBClient_put
        (JNIEnv *, jobject, jlong, jbyteArray, jbyteArray);

/*
 * Class:     site_ycsb_db_grpc_rocksdb_GRPCRocksDBClient
 * Method:    delete
 * Signature: (J[B)I
 */
JNIEXPORT jint JNICALL Java_site_ycsb_db_grpc_rocksdb_GRPCRocksDBClient_delete
        (JNIEnv *, jobject, jlong, jbyteArray);

/*
 * Class:     site_ycsb_db_grpc_rocksdb_GRPCRocksDBClient
 * Method:    scan
 * Signature: (J[BIZ)Lsite/ycsb/db/grpc/rocksdb/GRPCRocksDBClient/KVPairs;
 */
JNIEXPORT jobject JNICALL Java_site_ycsb_db_grpc_rocksdb_GRPCRocksDBClient_scan
        (JNIEnv *, jobject, jlong, jbyteArray, jint, jboolean);

/*
 * Class:     site_ycsb_db_grpc_rocksdb_GRPCRocksDBClient
 * Method:    disconnect
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_site_ycsb_db_grpc_rocksdb_GRPCRocksDBClient_disconnect
        (JNIEnv *, jobject, jlong);

#ifdef __cplusplus
}
#endif
#endif
