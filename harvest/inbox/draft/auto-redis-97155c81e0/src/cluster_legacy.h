// AUTO-DRAFT from redis/redis PR #13637
#ifndef CLUSTER_LEGACY_H
#define CLUSTER_LEGACY_H

#define CLUSTER_NODE_MEET 128     /* Send a MEET message to this node */
#define CLUSTER_NODE_MIGRATE_TO 256 /* Master eligible for replica migration. */
#define CLUSTER_NODE_NOFAILOVER 512 /* Slave will not try to failover. */
#define CLUSTER_NODE_NULL_NAME "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"

#define nodeIsSlave(n) ((n)->flags & CLUSTER_NODE_SLAVE)
#define nodeTimedOut(n) ((n)->flags & CLUSTER_NODE_PFAIL)
#define nodeFailed(n) ((n)->flags & CLUSTER_NODE_FAIL)
#define nodeCantFailover(n) ((n)->flags & CLUSTER_NODE_NOFAILOVER)

/* This structure represent elements of node->fail_reports. */
typedef struct clusterNodeFailReport {
