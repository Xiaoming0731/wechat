#ifndef FRIENDPAGE_H
#define FRIENDPAGE_H

#include <QWidget>
#include "chatservice.h"  // FriendInfo

namespace Ui {
class FriendPage;
}

class FriendPage : public QWidget
{
    Q_OBJECT

public:
    explicit FriendPage(QWidget *parent = nullptr);
    ~FriendPage();

    // 渲染指定好友资料到右侧资料页
    void SetFriend(const FriendInfo &info);

signals:
    // 点击「聊天」按钮，通知 ChatDialog 切到和这个好友的聊天
    void chatButtonClicked(int friendId);
    // 点设置按钮 → 弹"改好友备注"
    void settingClicked(int friendId, const QString &originalName, const QString &currentRemark);

private slots:
    // 由 Qt Creator 设计器或 setupUi 自动连接到 chat_btn 的 clicked 信号
    void on_chat_btn_clicked();

private:
    Ui::FriendPage *ui;
    FriendInfo _friend;
};

#endif // FRIENDPAGE_H
