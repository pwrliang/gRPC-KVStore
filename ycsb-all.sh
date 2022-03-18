#!/usr/bin/env bash
export WORKLOADS=""

export GRPC_PLATFORM_TYPE=TCP
./ycsb.sh ycsb/hosts

export GRPC_PLATFORM_TYPE=RDMA_BP
./ycsb.sh ycsb/hosts

export GRPC_PLATFORM_TYPE=RDMA_EVENT
./ycsb.sh ycsb/hosts

