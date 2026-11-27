#ifndef NEWFRIENDPAGE_H
#define NEWFRIENDPAGE_H

#include <QWidget>
#include <QMap>

#include "chatservice.h"   // ApplyInfo 结构体在这里定义

namespace Ui {
class NewFriendPage;
}

class FriendApplyWid;

class NewFriendPage : public QWidget
{
    Q_OBJECT

public:
    explicit NewFriendPage(QWidget *parent = nullptr);
    ~NewFriendPage();

private slots:
    // ChatService 信号 → 刷新列表
    void onFriendRequestsReady(const QVector<ApplyInfo> &list);
    void onHandleRequestResult(int reqid, bool success);
    void onFriendRequestNotify(int reqid, int fromId, const QString &fromName, const QString &applyMsg);

    // friendapplywid 同意按钮 → 本地置灰 + 调 ChatService 处理
    void onAcceptClicked(int reqid);

signals:
    // 同意好友申请后，请求 ChatDialog 弹"改好友备注"
    void openSetRemarkRequested(int friendId,
                                const QString &originalName,
                                const QString &currentRemark);

private:
    // 用服务端回包整体刷新（头插，最新的在顶部）
    void refreshList(const QVector<ApplyInfo> &list);

    // 创建一个 FriendApplyWid 并插入到指定位置（默认头插）
    void insertItem(const ApplyInfo &info, int row = 0);

    Ui::NewFriendPage *ui;
    // reqid → 对应的 FriendApplyWid*，用于处理结果时精确找到条目
    QMap<int, FriendApplyWid*> _widMap;
};

#endif // NEWFRIENDPAGE_H
