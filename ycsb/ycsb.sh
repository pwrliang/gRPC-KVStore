#!/usr/bin/env bash
set -e
SCRIPT_DIR=$(dirname "$0")
HOSTS_PATH="$SCRIPT_DIR/hosts"
DB_PREFIX="$(realpath ~)/rocks.db"
DB_PREFIX="/tmp/rocks.db"
SERVER=$(head -n 1 "$HOSTS_PATH")
NP=$(awk '{ sum += $1 } END { print sum }' <(tail -n +2 ycsb/hosts |cut -d"=" -f2,2))
LOGS_PATH=$(realpath "$SCRIPT_DIR/logs_$NP")
if [[ -z "$WORKLOADS" ]]; then
  WORKLOADS="workloada workloadb workloadc workloadd workloade workloadf tsworkloada"
fi

if [[ ! -f "$YCSB_HOME/bin/ycsb" ]]; then
  echo "Invalid YCSB_HOME"
  exit 1
fi

if [[ ! -f "$KVSTORE_HOME/kv_store" ]]; then
  echo "Invalid KVSTORE_HOME"
  exit 1
fi

cmd="$1"
profile_path="$2"
if [[ -f "$profile_path" ]]; then
  profile_path=$(realpath "$profile_path")
else
  echo "Invalid profile $profile_path"
  exit 1
fi
mkdir -p "$LOGS_PATH"

ssh "$SERVER" 'pkill kv_store || while $(pkill -0 kv_store 2>/dev/null); do continue; done'

for workload in ${WORKLOADS}; do
  db_file="${DB_PREFIX}/${workload}"
  if [[ $cmd == "load" ]]; then
    # remove database
    echo "Remove $db_file on $SERVER"
    ssh "$SERVER" rm -rf "$db_file"
    mkdir -p "$db_file"
    # Launch server
    mpirun -n 1 -host "$SERVER" "$KVSTORE_HOME"/kv_store -server -db_file="$db_file" &
    echo "Loading $workload"
    # Load data
    mpirun -n 1 -wdir "$YCSB_HOME" -host "$SERVER" \
        python2 $YCSB_HOME/bin/ycsb load grpcrocksdb \
          -jvm-args="-Djava.library.path=${KVSTORE_HOME}" \
          -P "$YCSB_HOME/workloads/$workload" \
          -P "$profile_path" \
          -s -p grpc.addr="$SERVER:12345"
  elif [[ $cmd == "run" ]]; then
    mpirun -n 1 -host "$SERVER" "$KVSTORE_HOME"/kv_store -server -db_file="$db_file" &

    rm -f "$LOGS_PATH/${workload}.log"
    tail -n +2 "$HOSTS_PATH" > /tmp/clients
    echo "Running $workload with $NP clients"
    # Evaluate
    mpirun -wdir "$YCSB_HOME" -hostfile /tmp/clients \
        python2 $YCSB_HOME/bin/ycsb run grpcrocksdb \
        -jvm-args="-Djava.library.path=${KVSTORE_HOME}" \
        -P "$YCSB_HOME/workloads/$workload" \
        -P "$profile_path" \
        -s -p grpc.addr="$SERVER:12345" | tee -a "$LOGS_PATH/${workload}.log" 2>&1
  else
    echo "Bad parm"
    exit 1
  fi
  # Kill server
  ssh "$SERVER" 'pkill kv_store && while $(pkill -0 kv_store 2>/dev/null); do continue; done'
  sleep 1
done
