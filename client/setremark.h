#ifndef SETREMARK_H
#define SETREMARK_H

#include <QWidget>

namespace Ui {
class SetRemark;
}

class SetRemark : public QWidget
{
    Q_OBJECT

public:
    explicit SetRemark(QWidget *parent = nullptr);
    ~SetRemark();

    // 显示之前必须调一次，告诉弹窗"改谁 + 默认显示什么"
    //   isGroup=false → targetId 是好友 id，originalName 是好友昵称
    //   isGroup=true  → targetId 是群 id，originalName 是群名
    //   currentRemark → 当前备注（空字符串表示还没设过），默认优先显示它
    void setup(int targetId, bool isGroup,
               const QString &originalName,
               const QString &currentRemark);

signals:
    // 用户点了"确认"：把修改结果吐给上层
    void remarkConfirmed(int targetId, bool isGroup, const QString &newRemark);

private slots:
    void on_pushButton_clicked();   // 确认
    void on_pushButton_2_clicked(); // 取消

private:
    Ui::SetRemark *ui;
    int _targetId = 0;
    bool _isGroup = false;
};

#endif // SETREMARK_H
