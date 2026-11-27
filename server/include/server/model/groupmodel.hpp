#ifndef GROUPMODEL_H
#define GROUPMODLE_H

#include "group.hpp"
#include <string>
#include <vector>
using namespace std;


class GroupModel
{
public:
    bool createGroup(Group &group);
    // 加群失败（例如群不存在）返回 false
    bool addGroup(int userid,int groupid,string role);
    // 判断 userid 是否在 groupid 对应的群里
    bool isMember(int userid,int groupid);
    // 判断群是否存在
    bool isGroupExist(int groupid);
    // 更新用户对群的备注（只影响该用户自己的视图）
    bool updateGroupRemark(int userid,int groupid,const string &remark);
    vector<Group> queryGroup(int userid);
    vector<int> queryGroupUsers(int userid,int groupid);
};

#endif