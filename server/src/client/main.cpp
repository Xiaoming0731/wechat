#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <limits>
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "group.hpp"
#include "user.hpp"
#include "public.hpp"

User g_currentUser;
vector<User> g_currentFriendList;
vector<Group> g_currentGroupList;
std::thread g_readTask;   // 接收消息的子线程，注销时需要 join 让它退出

// 读取一个整数，输入非法时清空输入流状态并返回 false，避免菜单无限刷屏
static bool readInt(int &value)
{
    if (!(cin >> value))
    {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cerr << "invalid input!" << endl;
        return false;
    }
    cin.get();
    return true;
}

void showCurrentUserData();

void readTaskHandle(int clientfd);
string getCurrentTime();
void mainMenu(int clientfd);

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "command invalid! example:./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }

    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientfd == -1)
    {
        cerr << "socket create error" << endl;
        exit(-1);
    }

    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    if (connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)) == -1)
    {
        cerr << "connect server error" << endl;
        close(clientfd);
        exit(-1);
    }

    for (;;)
    {
        cout << "===============================" << endl;
        cout << "1:login" << endl;
        cout << "2:register" << endl;
        cout << "3:quit" << endl;
        cout << "4:change password" << endl;
        cout << "===============================" << endl;
        cout << "choice:";
        int choice = 0;
        if (!readInt(choice))
        {
            continue;
        }

        switch (choice)
        {
        case 1:
        {
            int id = 0;
            char pwd[50] = {0};
            cout << "please input userid:" << endl;
            cin >> id;
            cin.get();
            cout << "please input password:" << endl;
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send login message error!" << request << endl;
            }
            else
            {
                char buffer[1024] = {0};
                len = recv(clientfd, buffer, 1024, 0);
                if (len == -1)
                {
                    cerr << "recvive login response error!" << endl;
                }
                else
                {
                    json responsejs = json::parse(buffer);
                    if (responsejs["errno"] != 0)
                    {
                        cerr << responsejs["errmsg"] << endl;
                    }
                    else
                    {
                        g_currentUser.setId(responsejs["id"]);
                        g_currentUser.setName(responsejs["name"]);

                        if (responsejs.contains("friends"))
                        {
                            vector<string> vec = responsejs["friends"];
                            for (string &s : vec)
                            {
                                json js = json::parse(s);
                                User user;
                                user.setId(js["id"]);
                                user.setName(js["name"]);
                                user.setState(js["state"]);
                                g_currentFriendList.push_back(user);
                            }
                        }

                        if (responsejs.contains("groups"))
                        {
                            vector<string> vec1 = responsejs["groups"];
                            for (string &s : vec1)
                            {
                                json grpjs = json::parse(s);
                                Group group;
                                group.setId(grpjs["id"]);
                                group.setName(grpjs["groupname"]);
                                group.setDesc(grpjs["groupdesc"]);

                                vector<string> vec2 = grpjs["users"];
                                for (string &s : vec2)
                                {
                                    GroupUser user;
                                    json js = json::parse(s);
                                    user.setId(js["id"]);
                                    user.setName(js["name"]);
                                    user.setState(js["state"]);
                                    user.setRole(js["role"]);
                                    group.getUsers().push_back(user);
                                }

                                g_currentGroupList.push_back(group);
                            }
                        }
                        showCurrentUserData();
                        if (responsejs.contains("offlinemsg"))
                        {
                            vector<string> vec = responsejs["offlinemsg"];
                            for (string &s : vec)
                            {
                                json js = json::parse(s);
                                cout << js["time"] << "[" << js["id"] << "]" << js["name"] << "said:" << js["msg"] << endl;
                            }
                        }
                        g_readTask = std::thread(readTaskHandle, clientfd);
                        mainMenu(clientfd);
                    }
                }
            }
        }
        break;
        case 2:
        {
            char name[50] = {0};
            char pwd[50] = {0};
            cout << "please input username:" << endl;
            cin.getline(name, 50);
            cout << "please input password:" << endl;
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send reg message error" << endl;
            }
            else
            {
                char buffer[1024] = {0};
                len = recv(clientfd, buffer, 1024, 0);
                if (len == -1)
                {
                    cerr << "receive reg response error " << endl;
                }
                else
                {
                    json responsejs = json::parse(buffer);
                    if (responsejs["errno"] != 0)
                    {
                        cerr << name << "is already exist,register error!" << endl;
                    }
                    else
                    {
                        cout << name << "register success, userid is " << responsejs["id"] << ",do not forget it!" << endl;
                    }
                }
            }
        }
        break;
        case 3:
        {
            close(clientfd);
            exit(0);
        }
        case 4:
        {
            int id = 0;
            char pwd[50] = {0};
            cout << "please input userid:" << endl;
            cin >> id;
            cin.get();
            cout << "please input new password:" << endl;
            cin.getline(pwd, 50);

            char confirm_pwd[50] = {0};
            cout << "please input new password again (confirm):" << endl;
            cin.getline(confirm_pwd, 50);

            json js;
            js["msgid"] = CHANGE_PASS_MSG;
            js["id"] = id;
            js["password"] = pwd;
            js["confirm_password"] = confirm_pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send change password error!" << endl;
            }
            else
            {
                char buffer[1024] = {0};
                len = recv(clientfd, buffer, 1024, 0);
                if (len != -1)
                {
                    json responsejs = json::parse(buffer);
                    if (responsejs["errno"] == 0)
                    {
                        cout << "change password success!" << endl;
                    }
                    else
                    {
                        cerr << "change password failed: " << responsejs.value("errmsg", string()) << endl;
                    }
                }
            }
        }
        break;
        default:
        {
            cerr << "invalid input!" << endl;
            break;
        }
        }
    }
    return 0;
}

// 打印当前登录用户信息、好友列表和群列表
void showCurrentUserData()
{
    cout << "======================login user======================" << endl;
    cout << "current login user => id:" << g_currentUser.getId() << " name:" << g_currentUser.getName() << endl;
    cout << "----------------------friend list---------------------" << endl;
    if (!g_currentFriendList.empty())
    {
        for (User &user : g_currentFriendList)
        {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }
    cout << "----------------------group list----------------------" << endl;
    if (!g_currentGroupList.empty())
    {
        for (Group &group : g_currentGroupList)
        {
            cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            for (GroupUser &user : group.getUsers())
            {
                cout << "  " << user.getId() << " " << user.getName() << " " << user.getState() << " " << user.getRole() << endl;
            }
        }
    }
    cout << "======================================================" << endl;
}

// 获取当前时间字符串
string getCurrentTime()
{
    time_t tt = chrono::system_clock::to_time_t(chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return string(date);
}

// 子线程：循环接收服务端推送的消息
void readTaskHandle(int clientfd)
{
    for (;;)
    {
        char buffer[4096] = {0};
        int len = recv(clientfd, buffer, sizeof(buffer) - 1, 0);
        if (len == -1 || len == 0)
        {
            close(clientfd);
            exit(0);
        }

        json js;
        try
        {
            js = json::parse(buffer);
        }
        catch (const exception &e)
        {
            cerr << "parse server message failed: " << e.what() << endl;
            continue;
        }

        int msgtype = js["msgid"];

        if (ONE_CHAT_MSG == msgtype)
        {
            cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>() << " said: " << js["msg"].get<string>() << endl;
            continue;
        }

        if (GROUP_CHAT_MSG == msgtype)
        {
            cout << "group[" << js["groupid"] << "] " << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>() << " said: " << js["msg"].get<string>() << endl;
            continue;
        }

        // 单聊失败的应答（例如对方不是好友）
        if (ONE_CHAT_MSG_ACK == msgtype)
        {
            cerr << "chat failed: " << js.value("errmsg", string()) << " (to " << js["to"] << ")" << endl;
            continue;
        }

        if (GROUP_CHAT_MSG_ACK == msgtype)
        {
            cerr << "group chat failed: " << js.value("errmsg", string()) << " (group " << js["groupid"] << ")" << endl;
            continue;
        }

        if (ADD_FRIEND_MSG_ACK == msgtype)
        {
            cout << "[add friend] errno=" << js["errno"] << " " << js.value("errmsg", string()) << endl;
            continue;
        }

        if (GET_FRIEND_REQ_MSG_ACK == msgtype)
        {
            vector<string> vec = js["requests"];
            if (vec.empty())
            {
                cout << "[friend requests] no pending request" << endl;
            }
            else
            {
                cout << "[friend requests] " << vec.size() << " pending:" << endl;
                for (string &s : vec)
                {
                    json item = json::parse(s);
                    cout << "  reqid=" << item["reqid"]
                         << " from " << item["from_id"] << " " << item["from_name"].get<string>()
                         << " [" << item["apply_msg"].get<string>() << "]"
                         << " " << item["time"].get<string>() << endl;
                }
            }
            continue;
        }

        if (HANDLE_FRIEND_REQ_MSG_ACK == msgtype)
        {
            cout << "[handle friend request] errno=" << js["errno"] << " " << js.value("errmsg", string()) << endl;
            continue;
        }

        // 别人给你发好友申请时的实时提醒（待处理列表用菜单 6 查）
        if (FRIEND_REQ_NOTIFY_MSG == msgtype)
        {
            cout << "[new friend request] from " << js["from_id"] << " " << js["from_name"].get<string>()
                 << " [" << js.value("apply_msg", string()) << "]"
                 << " " << js.value("time", string()) << endl;
            continue;
        }

        if (CREATE_GROUP_MSG_ACK == msgtype)
        {
            cout << "[create group] errno=" << js["errno"] << " " << js.value("errmsg", string());
            if (js.value("errno", 1) == 0)
            {
                cout << ", groupid=" << js["groupid"];
            }
            cout << endl;
            continue;
        }

        if (ADD_GROUP_MSG_ACK == msgtype)
        {
            cout << "[add group] errno=" << js["errno"] << " " << js.value("errmsg", string())
                 << " (group " << js["groupid"] << ")" << endl;
            continue;
        }

        if (LOGINOUT_MSG_ACK == msgtype)
        {
            return;   // 注销成功，结束接受消息的线程，socket 保留给下一次登录
        }
    }
}

// void help(int fd = 0, string str = "");
// // "chat" command handler
// void chat(int, string);
// // "addfriend" command handler
// void addfriend(int, string);
// // "creategroup" command handler
// void creategroup(int, string);
// // "addgroup" command handler
// void addgroup(int, string);
// // "groupchat" command handler
// void groupchat(int, string);
// // "loginout" command handler
// void loginout(int, string);

// 系统支持的客户端命令列表
// unordered_map<string, string> commandMap = {
//     {"help", "显示所有支持的命令，格式help"},
//     {"chat", "一对一聊天，格式chat:friendid:message"},
//     {"addfriend", "添加好友，格式addfriend:friendid"},
//     {"creategroup", "创建群组，格式creategroup:groupname:groupdesc"},
//     {"addgroup", "加入群组，格式addgroup:groupid"},
//     {"groupchat", "群聊，格式groupchat:groupid:message"},
//     {"loginout", "注销，格式loginout"}};

// // 注册系统支持的客户端命令处理
// unordered_map<string, function<void(int, string)>> commandHandlerMap = {
//     {"help", help},
//     {"chat", chat},
//     {"addfriend", addfriend},
//     {"creategroup", creategroup},
//     {"addgroup", addgroup},
//     {"groupchat", groupchat},
//     {"loginout", loginout}};

// void help(int, string)
// {
//     cout << "show command list >>> " << endl;
//     for (auto &p : commandMap)
//     {
//         cout << p.first << " : " << p.second << endl;
//     }
//     cout << endl;
// }
// void chat(int clientfd, string str)
// {
//     int idx = str.find(":"); // friendid:message
//     if (-1 == idx)
//     {
//         cerr << "chat command invalid!" << endl;
//         return;
//     }

//     int friendid = atoi(str.substr(0, idx).c_str());
//     string message = str.substr(idx + 1, str.size() - idx);

//     json js;
//     js["msgid"] = ONE_CHAT_MSG;
//     js["id"] = g_currentUser.getId();
//     js["name"] = g_currentUser.getName();
//     js["toid"] = friendid;
//     js["msg"] = message;
//     js["time"] = getCurrentTime();
//     string buffer = js.dump();

//     int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
//     if (-1 == len)
//     {
//         cerr << "send chat msg error -> " << buffer << endl;
//     }
// }

// void addfriend(int clientfd, string str)
// {
//      int friendid = atoi(str.c_str());
//     json js;
//     js["msgid"] = ADD_FRIEND_MSG;
//     js["id"] = g_currentUser.getId();
//     js["friendid"] = friendid;
//     string buffer = js.dump();

//     int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
//     if (-1 == len)
//     {
//         cerr << "send addfriend msg error -> " << buffer << endl;
//     }
// }

// void creategroup(int, string)
// {
// }

// void addgroup(int, string)
// {
// }

// void groupchat(int, string)
// {
// }

// void loginout(int, string)
// {
// }

// void mainMenu(int clientfd)
// {
//     help();
//     char buffer[1024] = {0};
//     for (;;)
//     {
//         cin.getline(buffer, 1024);
//         string commandbuf(buffer);
//         string command;
//         int idx = commandbuf.find(":");

//         if (idx == -1)
//         {
//             command = commandbuf;
//         }
//         else
//         {
//             command = commandbuf.substr(0, idx);
//         }
//         auto it = commandHandlerMap.find(command);
//         if (it == commandHandlerMap.end())
//         {
//             cerr << "invalid input command!" << endl;
//             continue;
//         }
//         it->second(clientfd,commandbuf.substr(idx+1,commandbuf.size()-idx));
//     }
// }
// 登录后的主菜单

void mainMenu(int clientfd)
{
    // help();

    for (;;)
    {
        cout << "================= main menu =================" << endl;
        cout << "1:chat" << endl;
        cout << "2:add friend" << endl;
        cout << "3:create group" << endl;
        cout << "4:add group" << endl;
        cout << "5:group chat" << endl;
        cout << "6:view friend requests" << endl;
        cout << "7:handle friend request" << endl;
        cout << "8:logout" << endl;
        cout << "=============================================" << endl;
        cout << "choice:";

        int choice = 0;
        if (!readInt(choice))
        {
            continue;
        }

        switch (choice)
        {
        case 1:
        {
            int friendid = 0;
            cout << "please input friendid:" << endl;
            if (!readInt(friendid))
            {
                break;
            }

            char message[1024] = {0};
            cout << "please input chat message:" << endl;
            cin.getline(message, 1024);

            json js;
            js["msgid"] = ONE_CHAT_MSG;
            js["id"] = g_currentUser.getId();
            js["name"] = g_currentUser.getName();
            js["to"] = friendid;
            js["msg"] = message;
            js["time"] = getCurrentTime();

            string request = js.dump();
            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send chat message error:" << request << endl;
            }
        }
        break;
        case 2:
        {
            int friendid = 0;
            cout << "please input friendid:" << endl;
            if (!readInt(friendid))
            {
                break;
            }

            json js;
            js["msgid"] = ADD_FRIEND_MSG;
            js["id"] = g_currentUser.getId();
            js["friendid"] = friendid;

            string request = js.dump();
            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send add friend message error:" << request << endl;
            }
            else
            {
                cout << "add friend request sent, relogin to refresh friend list!" << endl;
            }
        }
        break;
        case 3:
        {
            char groupname[50] = {0};
            char groupdesc[256] = {0};
            cout << "please input group name:" << endl;
            cin.getline(groupname, 50);
            cout << "please input group description:" << endl;
            cin.getline(groupdesc, 256);

            json js;
            js["msgid"] = CREATE_GROUP_MSG;
            js["id"] = g_currentUser.getId();
            js["groupname"] = groupname;
            js["groupdesc"] = groupdesc;

            string request = js.dump();
            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send create group message error:" << request << endl;
            }
            else
            {
                cout << "create group request sent, waiting server ack..." << endl;
            }
        }
        break;
        case 4:
        {
            int groupid = 0;
            cout << "please input groupid:" << endl;
            if (!readInt(groupid))
            {
                break;
            }

            json js;
            js["msgid"] = ADD_GROUP_MSG;
            js["id"] = g_currentUser.getId();
            js["groupid"] = groupid;

            string request = js.dump();
            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send add group message error:" << request << endl;
            }
            else
            {
                cout << "add group request sent, waiting server ack..." << endl;
            }
        }
        break;
        case 5:
        {
            int groupid = 0;
            cout << "please input groupid:" << endl;
            if (!readInt(groupid))
            {
                break;
            }

            char message[1024] = {0};
            cout << "please input chat message:" << endl;
            cin.getline(message, 1024);

            json js;
            js["msgid"] = GROUP_CHAT_MSG;
            js["id"] = g_currentUser.getId();
            js["name"] = g_currentUser.getName();
            js["groupid"] = groupid;
            js["msg"] = message;
            js["time"] = getCurrentTime();

            string request = js.dump();
            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send group chat message error:" << request << endl;
            }
        }
        break;
        case 6:
        {
            json js;
            js["msgid"] = GET_FRIEND_REQ_MSG;
            js["id"] = g_currentUser.getId();

            string request = js.dump();
            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send query friend request error:" << request << endl;
            }
        }
        break;
        case 7:
        {
            int reqid = 0;
            cout << "please input reqid (see list from menu 6):" << endl;
            if (!readInt(reqid))
            {
                break;
            }

            int accept = 0;
            cout << "1:accept  0:reject" << endl;
            if (!readInt(accept))
            {
                break;
            }

            json js;
            js["msgid"] = HANDLE_FRIEND_REQ_MSG;
            js["id"] = g_currentUser.getId();
            js["reqid"] = reqid;
            js["accept"] = (accept != 0);

            string request = js.dump();
            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send handle friend request error:" << request << endl;
            }
            else
            {
                cout << "handle friend request sent, relogin to refresh friend list!" << endl;
            }
        }
        break;
        case 8:
        {
            json js;
            js["msgid"] = LOGINOUT_MSG;
            js["id"] = g_currentUser.getId();

            string request = js.dump();
            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send loginout message error:" << request << endl;
            }

            // 等接收线程收到注销应答后退出，避免它抢走下一次登录的响应
            if (g_readTask.joinable())
            {
                g_readTask.join();
            }

            // 清空本地登录信息，返回登录菜单
            g_currentUser = User();
            g_currentFriendList.clear();
            g_currentGroupList.clear();
            return;
        }
        default:
        {
            cerr << "invalid input!" << endl;
        }
        break;
        }
    }
}