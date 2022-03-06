#!/usr/bin/env bash
set -e
SCRIPT_DIR=$(dirname "$0")
HOSTS_PATH="$SCRIPT_DIR/hosts"
DB_PREFIX="/tmp/rocks.db"
SERVER=$(head -n 1 "$HOSTS_PATH")
FIRST_CLIENT=$(head -n 2 "$HOSTS_PATH" | tail -n 1 | cut -d" " -f1,1)
NP=$(awk '{ sum += $1 } END { print sum }' <(tail -n +2 "$HOSTS_PATH" |cut -d"=" -f2,2))
MPI_LIB=$(realpath "$(which mpirun|xargs dirname)"/../lib)
ASNYC=false

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

for i in "$@"; do
  case $i in
    -c=*|--cmd=*)
      CMD="${i#*=}"
      shift
      ;;
    -p=*|--profile=*)
      PROFILE_PATH="${i#*=}"
      shift
      ;;
    --async)
      ASNYC=true
      shift
      ;;
    -*|--*)
      echo "Unknown option $i"
      exit 1
      ;;
    *)
      ;;
  esac
done

if [[ -f "$PROFILE_PATH" ]]; then
  PROFILE_PATH=$(realpath "$PROFILE_PATH")
else
  echo "Invalid profile $PROFILE_PATH"
  exit 1
fi

LOG_PATH=$(realpath "$SCRIPT_DIR/logs_$NP")
if [[ $ASYNC == true ]]; then
  LOG_PATH="${LOG_PATH}_async"
else
  LOG_PATH="${LOG_PATH}_sync"
fi

if [[ -n "$LOG_SUFFIX" ]]; then
  LOG_PATH="${LOG_PATH}_${LOG_SUFFIX}"
fi
mkdir -p "$LOG_PATH"
export RDMA_VERBOSITY=ERROR

function start_server() {
  workload=$1
  db_file="${DB_PREFIX}/${workload}"
  # Launch server
  mpirun -x GRPC_PLATFORM_TYPE -x RDMA_VERBOSITY \
         -n 1 -host "$SERVER" \
         "$KVSTORE_HOME"/kv_store -server -async=$ASNYC -db_file="$db_file" &
}

function kill_server() {
  ssh "$SERVER" 'if ps aux | pgrep kv_store >/dev/null; then pkill kv_store && while $(pkill -0 kv_store 2>/dev/null); do continue; done; fi'
#  ssh "$SERVER" 'ps aux|pgrep kv_store|xargs kill -9 || true'
}

function load() {
    workload=$1
    db_file="${DB_PREFIX}/${workload}"
    if ssh "$SERVER" mkdir -p "$db_file"; then
      start_server "$workload"
      echo "Loading $workload"
      mpirun -x GRPC_PLATFORM_TYPE -x RDMA_VERBOSITY -n 1 -wdir "$YCSB_HOME" -host "$FIRST_CLIENT" \
          python2 "$YCSB_HOME"/bin/ycsb load grpcrocksdb \
            -jvm-args="-Djava.library.path=${KVSTORE_HOME}:${MPI_LIB}" \
            -P "$YCSB_HOME/workloads/$workload" \
            -P "$PROFILE_PATH" \
            -s -p grpc.addr="$SERVER:12345"
      kill_server
    else
      echo "Failed to create $db_file"
      exit 1
    fi
}

for workload in ${WORKLOADS}; do
  db_file="${DB_PREFIX}/${workload}"
  curr_log_path="$LOG_PATH/${workload}.log"
  kill_server
  if [[ $CMD == "run" ]]; then
    if [[ -f "$curr_log_path" ]]; then
        echo "$curr_log_path exists, skip"
      else
        while true; do
        # remove database
          echo "Remove $db_file on $SERVER"
          ssh "$SERVER" rm -rf "$db_file"

          load "$workload"

#        if ! ssh "$SERVER" test -d "$db_file"; then
#          echo "Database $db_file does not exist, load it"
#          load "$workload"
#        fi
          exit 0
          start_server "$workload"

          tail -n +2 "$HOSTS_PATH" > /tmp/clients
          echo "============================= Running $workload with $NP clients"
          # Evaluate
          mpirun -x GRPC_PLATFORM_TYPE -x RDMA_VERBOSITY \
              -wdir "$YCSB_HOME" -np "$NP" -hostfile /tmp/clients \
              python2 "$YCSB_HOME"/bin/ycsb run grpcrocksdb \
              -jvm-args="-Djava.library.path=${KVSTORE_HOME}:${MPI_LIB}" \
              -P "$YCSB_HOME/workloads/$workload" \
              -P "$PROFILE_PATH" \
              -s -p grpc.addr="$SERVER:12345" | tee "${curr_log_path}.tmp" 2>&1
          kill_server

          if grep -q -e "FAILED" -e "A fatal error" < "${curr_log_path}.tmp"; then
            echo "Found error when evaluate workload $workload, retrying"
          else
            mv "${curr_log_path}.tmp" "${curr_log_path}"
            break
          fi
        done
      fi
  else
    echo "Bad command: $CMD"
  fi
done
