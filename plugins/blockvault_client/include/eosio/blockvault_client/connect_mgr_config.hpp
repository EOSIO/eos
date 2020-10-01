#ifndef __CONNECT_MANAGER_CONFIG_H 
#define __CONNECT_MANAGER_CONFIG_H 

#define CONNECT_MGR_THREAD_SAFE
#define LOAD_BALANCE
#define LOAD_BALANCE_RANDOM
#define CLUSTER_INIT_FILE_PATH        "~/blockvault_cluster.ini"
#define ALIGN_BYTES          8

#define ASYNC_SEND_QUEUE_SIZE          1024
#define ASYNC_RECEIVE_QUEUE_SIZE     1024

// {FULL_CONNECTION | RE_CONNECTION}  only define one of them
// FULL_CONNECTION hold all connection to cluster follower, and also one the connection to leader.   n + 1
// RE_CONNECTION hold one connection to cluster follower, and one connection to leader.                 1 + 1
//#define FULL_CONNECTION
#define RE_CONNECTION


#endif   //  end of __CONNECT_MANAGER_CONFIG_H