#ifndef USER_H
#define USER_H
#include <string>
using namespace std;


class User
{
public:
    User(int id=-1,string name="",string password="",string state="offline",string avatar=""){
        this->id=id;
        this->name=name;
        this->password=password;
        this->state=state;
        this->avatar=avatar;
    }
    void setId(int id){this->id=id;}
    void setName(string name){this->name=name;}
    void setPwd(string pwd){this->password=pwd;}
    void setState(string state){this->state=state;}
    void setAvatar(string avatar){this->avatar=avatar;}
    void setRemark(string remark){this->remark=remark;}

    int getId(){return this->id;}
    string getName(){return this->name;}
    string getPwd(){return this->password;}
    string getState(){return this->state;}
    string getAvatar(){return this->avatar;}
    string getRemark(){return this->remark;}

private:
    int id;
    string name;
    string password;
    string state;
    string avatar;
    string remark;
};

#endif
