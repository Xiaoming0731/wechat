#ifndef GROUP_H
#define GROUP_H
#include "groupuser.hpp"
#include <string>
#include <vector>
using namespace std;

class Group
{
public:
    Group(int id=-1,string name="",string desc="",string remark="")
    {
        this->id=id;
        this->name=name;
        this->desc=desc;
        this->remark=remark;
    }
    // ~Group();

    void setId(int id){this->id=id;}
    void setName(string name){this->name=name;}
    void setDesc(string desc){this->desc=desc;}
    // 当前用户对该群的备注（存在 groupuser.remark，不同用户可以不同）
    void setRemark(string remark){this->remark=remark;}

    int getId(){return this->id;}
    string getName(){return this->name;}
    string getDesc(){return this->desc;}
    string getRemark(){return this->remark;}
    vector<GroupUser>& getUsers(){return this->users;}
    

private:
    int id;
    string name;
    string desc;
    string remark;
    vector<GroupUser> users;

};

#endif