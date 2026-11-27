#include "forgetpasswordpage.h"
#include "ui_forgetpasswordpage.h"
#include "chatservice.h"
#include "iconswitchbtn.h"

#include <QMessageBox>

ForgetPasswordPage::ForgetPasswordPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ForgetPasswordPage)
{
    ui->setupUi(this);

    ChatService *service = ChatService::instance();

    // 取消 → 回登录页
    connect(ui->cancel_change_btn, &QPushButton::clicked, this, &ForgetPasswordPage::switchLogin);
    // 确认修改密码
    connect(ui->confirm_change_btn, &QPushButton::clicked, this, &ForgetPasswordPage::onConfirmClicked);

    // IconSwitchBtn：新密码显隐
    auto setupEyeBtn = [&](IconSwitchBtn *btn, QLineEdit *target)
    {
        btn->setIcons(
            ":/new/prefix1/res/unvisible.png",
            ":/new/prefix1/res/unvisible_hover.png",
            ":/new/prefix1/res/unvisible.png",
            ":/new/prefix1/res/visible.png",
            ":/new/prefix1/res/visible_hover.png",
            ":/new/prefix1/res/visible.png"
        );
        btn->setFixedSize(24, 24);
        target->setEchoMode(QLineEdit::Password);
        connect(btn, &IconSwitchBtn::toggled, this, [=](bool checked)
        {
            target->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
        });
    };
    setupEyeBtn(ui->password_eye_btn, ui->password_edit);
    setupEyeBtn(ui->confirm_password_eye_btn, ui->confirm_password_edit);

    // 接口响应
    connect(service, &ChatService::changePassOk, this, [this]()
    {
        ui->confirm_change_btn->setEnabled(true);
        QMessageBox::information(window(), QStringLiteral("提示"),
                                 QStringLiteral("密码修改成功，请重新登录"));
        emit switchLogin();
    });
    connect(service, &ChatService::changePassFailed, this, [this](const QString &errmsg)
    {
        ui->confirm_change_btn->setEnabled(true);
        QMessageBox::warning(window(), QStringLiteral("修改失败"), errmsg);
    });
    connect(service, &ChatService::networkError, this, [this](const QString &errmsg)
    {
        ui->confirm_change_btn->setEnabled(true);
        QMessageBox::warning(window(), QStringLiteral("网络错误"), errmsg);
    });
}

ForgetPasswordPage::~ForgetPasswordPage()
{
    delete ui;
}

void ForgetPasswordPage::onConfirmClicked()
{
    bool ok = false;
    const int userid = ui->lineEdit->text().trimmed().toInt(&ok);
    const QString pwd = ui->password_edit->text();
    const QString pwd2 = ui->confirm_password_edit->text();

    if (!ok || userid <= 0)
    {
        QMessageBox::warning(window(), QStringLiteral("提示"), QStringLiteral("请输入正确的用户ID"));
        return;
    }
    if (pwd.isEmpty())
    {
        QMessageBox::warning(window(), QStringLiteral("提示"), QStringLiteral("请输入新密码"));
        return;
    }
    if (pwd != pwd2)
    {
        QMessageBox::warning(window(), QStringLiteral("提示"), QStringLiteral("两次输入的密码不一致"));
        return;
    }

    ui->confirm_change_btn->setEnabled(false);
    ChatService::instance()->changePassword(userid, pwd, pwd2);
}
