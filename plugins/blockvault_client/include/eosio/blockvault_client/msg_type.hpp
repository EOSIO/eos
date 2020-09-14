#ifndef _MESSAGE_TYPE_HPP_
#define _MESSAGE_TYPE_HPP_

#include <string>

namespace blockvault_client {


enum msg_type {
    request_vote_request            = 1,
    request_vote_response           = 2,
    append_entries_request          = 3,
    append_entries_response         = 4,
    client_request                  = 5,
    add_server_request              = 6,
    add_server_response             = 7,
    remove_server_request           = 8,
    remove_server_response          = 9,
    sync_log_request                = 10,
    sync_log_response               = 11,
    join_cluster_request            = 12,
    join_cluster_response           = 13,
    leave_cluster_request           = 14,
    leave_cluster_response          = 15,
    install_snapshot_request        = 16,
    install_snapshot_response       = 17,
    ping_request                    = 18,
    ping_response                   = 19,
    pre_vote_request                = 20,
    pre_vote_response               = 21,
    other_request                   = 22,
    other_response                  = 23,
    priority_change_request         = 24,
    priority_change_response        = 25,
    reconnect_request               = 26,
    reconnect_response              = 27,
    custom_notification_request     = 28,
    custom_notification_response    = 29,

    ///////////////////////////////////////////////////////
    //// below message type is for debug and test only.
    fake_http_request               = 101,
    fake_http_response              = 102,
    fake_https_request              = 103,
    fake_https_response             = 104,
    fake_rpc_call                   = 105,
    fake_rpc_return                 = 106,
};

static bool is_valid_msg(msg_type type) {
    if ( type >= request_vote_request &&
         type <= other_response ) {
        return true;
    }
    return false;
}

// for tracing and debugging
static std::string msg_type_to_string(msg_type type)
{
    switch (type) {
    case request_vote_request:          return "request_vote_request";
    case request_vote_response:         return "request_vote_response";
    case append_entries_request:        return "append_entries_request";
    case append_entries_response:       return "append_entries_response";
    case client_request:                return "client_request";
    case add_server_request:            return "add_server_request";
    case add_server_response:           return "add_server_response";
    case remove_server_request:         return "remove_server_request";
    case remove_server_response:        return "remove_server_response";
    case sync_log_request:              return "sync_log_request";
    case sync_log_response:             return "sync_log_response";
    case join_cluster_request:          return "join_cluster_request";
    case join_cluster_response:         return "join_cluster_response";
    case leave_cluster_request:         return "leave_cluster_request";
    case leave_cluster_response:        return "leave_cluster_response";
    case install_snapshot_request:      return "install_snapshot_request";
    case install_snapshot_response:     return "install_snapshot_response";
    case ping_request:                  return "ping_request";
    case ping_response:                 return "ping_response";
    case pre_vote_request:              return "pre_vote_request";
    case pre_vote_response:             return "pre_vote_response";
    case other_request:                 return "other_request";
    case other_response:                return "other_response";
    case priority_change_request:       return "priority_change_request";
    case priority_change_response:      return "priority_change_response";
    case reconnect_request:             return "reconnect_request";
    case reconnect_response:            return "reconnect_response";
    case custom_notification_request:   return "custom_notification_request";
    case custom_notification_response:  return "custom_notification_response";
    default:
        return "unknown (" + std::to_string(static_cast<int>(type)) + ")";
    }
}

}

#endif //_MESSAGE_TYPE_HPP_

