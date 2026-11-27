#ifndef GLOBAL_H
#define GLOBAL_H

#include <QWidget>
#include <functional>
#include <QRegularExpression>
#include "QStyle"

extern std::function<void(QWidget*)> repolish;

enum ClickLbState{
    Normal=0,
    Selected=1
};


enum class ChatUIMode{
    SearchMode,
    ChatMode,
    ContactMode
};

enum ListItemType{
    CHAT_USER_ITEM,     // 聊天用户
    CONTACT_USER_ITEM,  // 联系人用户
    SEARCH_USER_ITEM,   // 搜索到的用户
    ADD_USER_TIP_ITEM,  // 提示添加用户
    INVALID_ITEM,       // 不可点击条目
    GROUP_TIP_ITEM,     // 分组提示条目
};

enum class ChatRole
{
    Self,
    Other
};
struct MsgInfo{
    QString msgFlag;//"text,image,file"
    QString content;//表示文件和图像的url，文本信息
    QPixmap pixmap;//文件和图片的缩略图
};

enum MsgStatus{
    UN_READ = 0,  //对方未读
    SEND_FAILED = 1,  //发送失败
    READED = 2  //对方已读
};




#endif // GLOBAL_H
