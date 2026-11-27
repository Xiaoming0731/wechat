#ifndef CHATUSERWID_H
#define CHATUSERWID_H

#include <QWidget>
#include <QPixmap>
#include <QLabel>
#include "listitembase.h"

namespace Ui {
class ChatUserWid;
}

class ChatUserWid : public ListItemBase
{
    Q_OBJECT

public:
    explicit ChatUserWid(QWidget *parent = nullptr);
    ~ChatUserWid();

    QSize sizeHint() const override {
        return QSize(250, 70); // 返回自定义的尺寸
    }

    void SetInfo(int friendId, const QString &name, const QPixmap &head, const QString &msg,
                 bool isGroup = false);
    // 只更新"最后一条消息"预览，不重建头像/名字
    void setChatText(const QString &msg);
    // 只更新显示名（备注变更时用），不重建头像/预览
    void setDisplayName(const QString &name);
    // 更新时间显示（服务端格式 yyyy-MM-dd HH:mm:ss，这里转成 HH:mm / 昨天 / MM-dd）
    void setTime(const QString &time);
    int friendId() const { return _friendId; }
    int groupId() const { return _groupId; }
    bool isGroup() const { return _groupId > 0; }

    // 临时会话：从好友页点「聊天」刚建出来的，还没发过任何消息
    void setTemp(bool on) { _temp = on; }
    bool isTemp() const { return _temp; }
    QString name() const { return _name; }

    // 由 ChatUserList 在选中项变化时调用，切换整行选中底色
    void setSelected(bool on);

    // 红点控制：ChatDialog 收到 sessionUnreadChanged 时调
    void setUnread(bool on);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    // 把红点对齐到 icon_lb 右上角（以 ChatUserWid 自身为坐标系）
    void updateRedPointGeometry();

    Ui::ChatUserWid *ui;
    QString _name;
    QString _msg;
    int _friendId = 0;
    int _groupId = 0;
    bool _temp = false;
    bool _unread = false;
    QLabel *_redPoint = nullptr;
};


#endif // CHATUSERWID_H
