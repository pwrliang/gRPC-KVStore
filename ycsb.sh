export RDMA_VERBOSITY=ERROR

rdma_mode=""
if [[ $GRPC_PLATFORM_TYPE == "RDMA_EVENT" ]]; then
  rdma_mode="event"
elif [[ $GRPC_PLATFORM_TYPE == "RDMA_BP" ]]; then
  rdma_mode="bp"
elif [[ $GRPC_PLATFORM_TYPE == "TCP" ]]; then
  rdma_mode="tcp"
else
  echo "Need valid ENV GRPC_PLATFORM_TYPE"
  exit 1
fi

export LOG_SUFFIX=$rdma_mode

if [[ $rdma_mode == "tcp" ]]; then
  export KVSTORE_HOME=/home/geng.161/Projects/gRPC-KVStore/build_original
  echo "TCP"
else
  export KVSTORE_HOME=/home/geng.161/Projects/gRPC-KVStore/build_rdma
  echo "RDMA"
fi
#./ycsb/ycsb.sh load "$(realpath ycsb/workload.dat)"
./ycsb/ycsb.sh run "$(realpath ycsb/workload.dat)"