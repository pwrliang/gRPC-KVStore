#!/usr/bin/env bash
SCRIPT_DIR=$(dirname "$0")
PROFILE_PATH=$(realpath "$SCRIPT_DIR/large.dat")
LOGS_PATH=$(realpath "$SCRIPT_DIR/logs")
HOSTS_PATH="$SCRIPT_DIR/hosts"
DB_PREFIX="$(realpath ~)/rocks.db"
SERVER=$(head -n 1 "$HOSTS_PATH")
WORKLOADS="workloada workloadb workloadc workloadd workloade workloadf"

if [[ ! -d "$DB_PREFIX" ]]; then
  mkdir -p "$DB_PREFIX"
fi

if [[ ! -f "$YCSB_HOME/bin/ycsb" ]]; then
  echo "Invalid YCSB_HOME"
  exit 1
fi

if [[ ! -f "$KVSTORE_HOME/kv_store" ]]; then
  echo "Invalid KVSTORE_HOME"
  exit 1
fi

cmd=$1
for workload in ${WORKLOADS}; do
  db_file="${DB_PREFIX}/${workload}"
  # Launch server
  mpirun -host "$SERVER" "$KVSTORE_HOME"/kv_store -server -db_file="$db_file" > /dev/null 2>&1 &
  if [[ $cmd == "load" ]]; then
    # remove database
    mpirun -host "$SERVER" rm -rf "$db_file"
    echo "Loading $workload"
    # Load data
    mpirun -wdir "$YCSB_HOME" -host "$SERVER" \
        python2 $YCSB_HOME/bin/ycsb load grpcrocksdb \
          -jvm-args="-Djava.library.path=${KVSTORE_HOME}" \
          -P "$YCSB_HOME/workloads/$workload" \
          -P "$PROFILE_PATH" \
          -s -p grpc.addr="$SERVER:12345" > /dev/null 2>&1
  elif [[ $cmd == "run" ]]; then
    rm -f "$LOGS_PATH/${workload}.log"
    tail -n +2 "$HOSTS_PATH" > /tmp/clients
    # Evaluate
    mpirun -wdir "$YCSB_HOME" -hostfile /tmp/clients \
        python2 $YCSB_HOME/bin/ycsb run grpcrocksdb \
        -jvm-args="-Djava.library.path=${KVSTORE_HOME}" \
        -P "$YCSB_HOME/workloads/$workload" \
        -P "$PROFILE_PATH" \
        -s -p grpc.addr="$SERVER:12345" | tee -a "$LOGS_PATH/${workload}.log" 2>&1
  else
    echo "Bad parm"
    exit 1
  fi
  # Kill server
  mpirun -host "$SERVER" pkill kv_store
done
