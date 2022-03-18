set -e
export RDMA_VERBOSITY=ERROR

rdma_mode=""
if [[ $GRPC_PLATFORM_TYPE == "RDMA_EVENT" ]]; then
  rdma_mode="event_new"
elif [[ $GRPC_PLATFORM_TYPE == "RDMA_BP" ]]; then
  rdma_mode="bp_new"
elif [[ $GRPC_PLATFORM_TYPE == "TCP" ]]; then
  rdma_mode="tcp_new"
else
  echo "Need valid ENV GRPC_PLATFORM_TYPE"
  exit 1
fi

export LOG_SUFFIX=$rdma_mode

if [[ $rdma_mode =~ tcp* ]]; then
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

#if [[ -z $HOSTS_PATH ]]; then
#  echo "Bad HOSTS_PATH"
#  exit 1
#fi

for n_clients in 1 2 4 8 16 32 64; do
  name_prefix=$(basename "$hostfile_template")
  hostfile="/tmp/$name_prefix.${RANDOM}"
  while [[ -f "$hostfile" ]]; do
    hostfile="/tmp/$name_prefix.${RANDOM}"
  done
  ./gen_hostfile.py ./ycsb/hosts $n_clients > "$hostfile"
  export HOSTS_PATH="$hostfile"
#  ./ycsb/ycsb.sh -c=run -p="$(realpath ycsb/workload.dat)"
  ./ycsb/ycsb.sh -c=run -p="$(realpath ycsb/workload.dat)" --async --thread=28
done
