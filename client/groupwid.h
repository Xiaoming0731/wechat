#ifndef GROUPWID_H
#define GROUPWID_H

#include <QWidget>

namespace Ui {
class GroupWid;
}

class GroupWid : public QWidget
{
    Q_OBJECT

public:
    explicit GroupWid(QWidget *parent = nullptr);
    ~GroupWid();
    void SetInfo(int groupId,const QString& name,const QPixmap &icon);
    int groupId()const{return _groupId;}

private:
    Ui::GroupWid *ui;
    int _groupId=0;
};

#endif // GROUPWID_H
