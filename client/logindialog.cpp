#include "logindialog.h"
#include "ui_logindialog.h"
#include "chatservice.h"

#include <QMessageBox>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    connect(ui->reg_btn,&QPushButton::clicked,this,&LoginDialog::switchReg);
    connect(ui->forget_btn,&QPushButton::clicked,this,&LoginDialog::switchForget);
    connect(ui->confirm_btn,&QPushButton::clicked,this,&LoginDialog::onLoginClicked);

    ChatService *service = ChatService::instance();

    connect(service, &ChatService::loginOk, this, [this](int, const QString &)
            {
                ui->confirm_btn->setEnabled(true);
                emit switchChat();
            });

    connect(service, &ChatService::loginFailed, this, [this](const QString &errmsg)
            {
                ui->confirm_btn->setEnabled(true);
                QMessageBox::warning(window(), QStringLiteral("登录失败"), errmsg);
            });

    connect(service, &ChatService::networkError, this, [this](const QString &errmsg)
            {
                ui->confirm_btn->setEnabled(true);
                QMessageBox::warning(window(), QStringLiteral("网络错误"), errmsg);
            });

    ui->password_btn->setIcons(
        ":/new/prefix1/res/unvisible.png",        // normal
        ":/new/prefix1/res/unvisible_hover.png",  // hover
        ":/new/prefix1/res/unvisible.png",        // press 没有专门按下图，复用normal图，不要传空！
        ":/new/prefix1/res/visible.png",          // checked
        ":/new/prefix1/res/visible_hover.png",    // checked-hover（修正拼写！）
        ":/new/prefix1/res/visible.png"           // checked-press 没有专门按下图，复用checked图，不要传空！
        );
    ui->password_btn->setFixedSize(24,24);
    // 默认密码隐藏，点小眼睛切换显隐
    ui->passedit->setEchoMode(QLineEdit::Password);
    connect(ui->password_btn, &IconSwitchBtn::toggled, this, [this](bool checked)
    {
        ui->passedit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
    });
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::onLoginClicked()
{
    bool ok = false;
    const int userid = ui->useredit->text().trimmed().toInt(&ok);
    const QString password = ui->passedit->text();

    if (!ok || userid <= 0)
    {
        QMessageBox::warning(window(), QStringLiteral("提示"), QStringLiteral("请输入正确的用户ID"));
        return;
    }
    if (password.isEmpty())
    {
        QMessageBox::warning(window(), QStringLiteral("提示"), QStringLiteral("请输入密码"));
        return;
    }

    ui->confirm_btn->setEnabled(false);
    ChatService::instance()->login(userid, password);
}
