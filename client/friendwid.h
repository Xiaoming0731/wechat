#ifndef FRIENDWID_H
#define FRIENDWID_H

#include <QWidget>

namespace Ui {
class friendwid;
}

class friendwid : public QWidget
{
    Q_OBJECT

public:
    explicit friendwid(QWidget *parent = nullptr);
    ~friendwid();

    // friendId 是 ChatService::friends() 里对应 FriendInfo 的 id，点击时回传给 ChatDialog 查资料
    void SetInfo(int friendId, const QString &name, const QPixmap &icon, const QString &state);

    int friendId() const { return _friendId; }

private:
    Ui::friendwid *ui;
    int _friendId = 0;
};

#endif // FRIENDWID_H
