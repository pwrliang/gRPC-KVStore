# YCSB Notes


`./bin/ycsb load grpcrocksdb -jvm-args="-Djava.library.path=/Users/liang/CLionProjects/gRPC-KVStore/cmake-build-debug" -s -P workloads/workloada -P large.dat -p grpc.addr="localhost:12345"`
`./bin/ycsb load grpcrocksdb -jvm-args="-Djava.library.path=/Users/liang/CLionProjects/gRPC-KVStore/cmake-build-debug" -s -P workloads/workloada -p grpc.addr="localhost:12345"`

`./bin/ycsb run grpcrocksdb -jvm-args="-Djava.library.path=/Users/liang/CLionProjects/gRPC-KVStore/cmake-build-debug" -s -P workloads/workloada -p grpc.addr="localhost:12345"`


`mvn -pl site.ycsb:grpcrocksdb-binding -am clean package`
