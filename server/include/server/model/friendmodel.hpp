#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H

#include "user.hpp"
#include<vector>
using namespace std;
class FriendModel
{
public:
    void insert(int userid,int friendid);

    // 判断两人是否为好友（friend 表双向各存一行，查单向即可）
    bool isFriend(int userid, int friendid);

    vector<User> query(int userid);

    // 更新用户对好友的备注（只改 userid→friendid 那行，对方备注不动）
    bool updateRemark(int userid, int friendid, const string &remark);
};

#endif