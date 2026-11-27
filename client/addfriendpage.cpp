#include "addfriendpage.h"
#include "ui_addfriendpage.h"

#include "chatservice.h"

#include <QMessageBox>

AddFriendPage::AddFriendPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AddFriendPage)
{
    ui->setupUi(this);

    // 申请结果回传 → 提示并关闭
    connect(ChatService::instance(), &ChatService::addFriendResult,
            this, &AddFriendPage::onAddFriendResult);
}

AddFriendPage::~AddFriendPage()
{
    delete ui;
}

void AddFriendPage::on_addfriend_confirm_btn_clicked()
{
    const QString idText = ui->lineEdit->text().trimmed();
    const QString applyMsg = ui->lineEdit_2->text().trimmed();

    // 输入校验
    if (idText.isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("请输入对方的用户ID"));
        return;
    }

    bool ok = false;
    const int friendId = idText.toInt(&ok);
    if (!ok || friendId <= 0)
    {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("用户ID必须是正整数"));
        return;
    }

    ChatService *service = ChatService::instance();
    if (!service->isLoggedIn())
    {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("当前未登录，无法发送好友申请"));
        return;
    }

    if (friendId == service->userId())
    {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("不能添加自己为好友"));
        return;
    }

    // 发出好友申请，结果由 onAddFriendResult 处理
    service->addFriend(service->userId(), friendId, applyMsg);
}

void AddFriendPage::on_addfriend_cancer_btn_clicked()
{
    // 取消：直接关闭窗口
    close();
}

void AddFriendPage::onAddFriendResult(int err, const QString &errmsg, int friendId)
{
    Q_UNUSED(friendId);

    if (err == 0)
    {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("好友申请已发送"));
        // 成功后清空输入，方便下次使用
        ui->lineEdit->clear();
        ui->lineEdit_2->clear();
        close();
    }
    else
    {
        QMessageBox::warning(this, QStringLiteral("发送失败"),
                             errmsg.isEmpty() ? QStringLiteral("未知错误") : errmsg);
    }
}
