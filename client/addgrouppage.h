#ifndef ADDGROUPPAGE_H
#define ADDGROUPPAGE_H

#include <QWidget>

namespace Ui {
class AddGroupPage;
}

class AddGroupPage : public QWidget
{
    Q_OBJECT

public:
    explicit AddGroupPage(QWidget *parent = nullptr);
    ~AddGroupPage();

signals:
    // 用户点"确定"时发出，groupid 取输入框内容
    void addGroupRequested(int groupid);

private slots:
    void onOkClicked();
    void onCancelClicked();

private:
    Ui::AddGroupPage *ui;
};

#endif // ADDGROUPPAGE_H
