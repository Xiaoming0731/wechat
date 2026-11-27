#ifndef ADDFRIENDPAGE_H
#define ADDFRIENDPAGE_H

#include <QWidget>

namespace Ui {
class AddFriendPage;
}

class AddFriendPage : public QWidget
{
    Q_OBJECT

public:
    explicit AddFriendPage(QWidget *parent = nullptr);
    ~AddFriendPage();

private slots:
    // Qt Auto-Connect：ui 里按钮 objectName 与槽签名严格匹配
    void on_addfriend_confirm_btn_clicked();
    void on_addfriend_cancer_btn_clicked();

    // ChatService 申请结果回调
    void onAddFriendResult(int err, const QString &errmsg, int friendId);

private:
    Ui::AddFriendPage *ui;
};

#endif // ADDFRIENDPAGE_H
