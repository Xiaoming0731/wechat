#ifndef PUBLIC_H
#define PUBLIC_H

enum EnMsgType{
    LOGIN_MSG=1,
    LOGIN_MSG_ACK,
    REG_MSG,
    REG_MSG_ACK,
    ONE_CHAT_MSG,
    ADD_FRIEND_MSG,

    CREATE_GROUP_MSG,
    ADD_GROUP_MSG,
    GROUP_CHAT_MSG,

    LOGINOUT_MSG,
    LOGINOUT_MSG_ACK,

    QUERY_HISTORY_MSG = 12,
    QUERY_HISTORY_MSG_ACK = 13,

    ONE_CHAT_MSG_ACK = 14,

    ADD_FRIEND_MSG_ACK = 15,
    GET_FRIEND_REQ_MSG = 16,
    GET_FRIEND_REQ_MSG_ACK = 17,
    HANDLE_FRIEND_REQ_MSG = 18,
    HANDLE_FRIEND_REQ_MSG_ACK = 19,

    GROUP_CHAT_MSG_ACK = 20,

    // 有人给你发好友申请时，服务端主动推给你（不落库，待处理列表以 friend_request 表为准）
    FRIEND_REQ_NOTIFY_MSG = 21,

    CREATE_GROUP_MSG_ACK = 22,
    ADD_GROUP_MSG_ACK = 23,

    UPDATE_REMARK_MSG,     // =24
    UPDATE_REMARK_MSG_ACK, // =25

    // 好友申请被同意后，服务端主动推给申请人：你的好友列表变了
    FRIEND_LIST_NOTIFY_MSG, // =26

    // 忘记密码/重置密码
    CHANGE_PASS_MSG,        // =27
    CHANGE_PASS_MSG_ACK     // =28
};

#endif
