#include <iostream>
#include <map>
#include <vector>
#include <string>
#include "json.hpp"
using json = nlohmann::json;
using namespace std;

string func1(){
    json js;
    js["msg_type"]=2;
    js["from"]="xiao ming";
    js["to"]="xiao liu";
    js["msg"]="hello,what are you doing?";

    string str=js.dump();
    return str;
}

string func2(){
    json js;
    js["msg_type"]={1,2,3,4,5};
    js["from"]="xiao ming";
    js["to"]="xiao liu";
    js["msg"]={{"xiao ming","hello,what are you doing?"},{"xiao liu","i am coding!"}};

    // string str=js.dump();
    return js.dump();
}

string func3(){
    json js;
    vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    js["list"]=vec;

    map<int,string> mp;
    mp.insert({1,"xiaoming"});
    mp.insert({2,"xiaoli"});
    mp.insert({3,"xiaozhang"});

    js["map"]=mp;

    string sendbuf=js.dump();
    // cout<<sendbuf.c_str()<<endl;

    return sendbuf;
}

int main(){
    string recerv_buf=func3();
    // func2();
    // func3();
    json jsbuf=json::parse(recerv_buf);
    // cout<<jsbuf["msg_type"]<<endl;
    // cout<<jsbuf["from"]<<endl;
    // cout<<jsbuf["to"]<<endl;
    // cout<<jsbuf["msg"]<<endl;

    // cout<<jsbuf["msg_type"]<<endl;
    // cout<<jsbuf["from"]<<endl;
    // cout<<jsbuf["to"]<<endl;
    // cout<<jsbuf["msg"]<<endl;

    cout<<jsbuf["list"]<<endl;
    cout<<jsbuf["map"]<<endl;
    return 0;
}