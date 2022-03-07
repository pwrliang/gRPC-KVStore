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

hostfile_template="$1"
hostfile_template=$(realpath "$hostfile_template")
if [[ ! -f "$hostfile_template" ]]; then
  echo "Bad hostfile $hostfile_template"
  exit 1
fi

for n_clients in 1 2 4 8 16 32 64; do
  name_prefix=$(basename "$hostfile_template")
  hostfile="/tmp/$name_prefix.${RANDOM}"
  while [[ -f "$hostfile" ]]; do
    hostfile="/tmp/$name_prefix.${RANDOM}"
  done
  sed -E "s/slots=[0-9]+/slots=${n_clients}/" < "$hostfile_template" > "$hostfile"
  export HOSTS_PATH="$hostfile"
  ./ycsb/ycsb.sh -c=run -p="$(realpath ycsb/workload.dat)"
  ./ycsb/ycsb.sh -c=run -p="$(realpath ycsb/workload.dat)" --async
done