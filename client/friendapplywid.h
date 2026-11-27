#ifndef FRIENDAPPLYWID_H
#define FRIENDAPPLYWID_H

#include <QWidget>

#include "chatservice.h"   // 为了拿到 ApplyInfo 结构体

namespace Ui {
class FriendApplyWid;
}

class FriendApplyWid : public QWidget
{
    Q_OBJECT

public:
    explicit FriendApplyWid(QWidget *parent = nullptr);
    ~FriendApplyWid();

    // 用申请数据填充控件（头像/昵称/申请消息），同时记住 reqid 供同意时上报
    void SetInfo(const ApplyInfo &info);

    // 按钮状态切换：同意成功后头插条目用，请求失败时恢复
    void markHandled();
    void markEnabled();

    // 外部需要拿到完整 ApplyInfo（比如同意后要弹 SetRemark，需要 fromId / fromName）
    const ApplyInfo &info() const { return _info; }

signals:
    // 点击"同意"按钮时发出，带 reqid；已处理状态下不发
    void acceptClicked(int reqid);

private:
    Ui::FriendApplyWid *ui;
    ApplyInfo _info;
    int _reqid;
    bool _handled;
};

#endif // FRIENDAPPLYWID_H
