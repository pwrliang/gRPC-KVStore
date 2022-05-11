set -e
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

export KVSTORE_HOME=

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
  ./gen_hostfile.py ./ycsb/hosts $n_clients > "$hostfile"
  export HOSTS_PATH="$hostfile"
  ./ycsb/ycsb.sh -c=run -p="$(realpath ycsb/workload.dat)" --async --thread=28
done
