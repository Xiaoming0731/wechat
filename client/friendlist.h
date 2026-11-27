#ifndef FRIENDLIST_H
#define FRIENDLIST_H

#include <QWidget>
#include <QLabel>

namespace Ui {
class FriendList;
}

class FriendList : public QWidget
{
    Q_OBJECT

public:
    explicit FriendList(QWidget *parent = nullptr);
    ~FriendList();

    // 清空列表并按好友数据重新填充
    void refreshFriends();

    // ChatService 的 friendApplyRedChanged 直接连到这个 slot
    // 点「新的朋友」后 ChatDialog 会调 clearApplyUnread，信号转发到这里
public slots:
    void setApplyRed(bool show);

signals:
    // 点击功能区按钮时发出，由 ChatDialog 决定弹出哪个页面
    void newFriendClicked();
    void addFriendClicked();
    void addGroupClicked();
    void createGroupClicked();
    void groupListClicked();

    // 点击具体好友条目时发出，friendId 是 ChatService::friends() 里对应 FriendInfo 的 id
    // ChatDialog 收到后查 FriendInfo、渲染 FriendPage
    void friendClicked(int friendId);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    Ui::FriendList *ui;
    // 叠在 newfriend_btn 上的红点，父对象是按钮本身，不参与布局
    QLabel *m_applyRedDot;
};

#endif // FRIENDLIST_H
